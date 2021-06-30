#include "hello_queue.h"
#include <minix/chardriver.h>
#include <minix/drivers.h>
#include <minix/ds.h>
#include <minix/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioc_hello_queue.h>

/* Function prototypes for the hello queue driver. */
static int helloq_open(devminor_t minor, int access, endpoint_t user_endpt);
static int helloq_close(devminor_t minor);
static ssize_t helloq_read(devminor_t minor, u64_t position, endpoint_t endpt,
                           cp_grant_id_t grant, size_t size, int flags,
                           cdev_id_t id);
static ssize_t helloq_write(devminor_t minor, u64_t pos, endpoint_t ep,
                            cp_grant_id_t gid, size_t size, int flags,
                            cdev_id_t id);
static int helloq_ioctl(devminor_t minor, unsigned long request,
                        endpoint_t endpt, cp_grant_id_t grant, int flags,
                        endpoint_t user_endpt, cdev_id_t id);

/* SEF functions and variables. */
static void sef_local_startup(void);
static int sef_cb_init(int type, sef_init_info_t *info);
static int sef_cb_lu_state_save(int);
static int lu_state_restore(void);

static struct chardriver helloq_tab = {.cdr_open = helloq_open,
                                       .cdr_close = helloq_close,
                                       .cdr_read = helloq_read,
                                       .cdr_write = helloq_write,
                                       .cdr_ioctl = helloq_ioctl};

/* State variables to save queue buffer and manage it. */
static char *queue_buffer;
static int queue_buffer_capacity;
static int queue_buffer_size;

static void resize_queue_buffer(size_t new_size) {
  char *new_queue_buffer = realloc(queue_buffer, new_size);
  if (new_queue_buffer == NULL) {
    free(queue_buffer);
    printf("Memory allocation failed in hello queue driver\n");
    exit(1);
  }
  queue_buffer = new_queue_buffer;
  queue_buffer_capacity = new_size;
}

static void set_initial_queue_value() {
  const char *initial_msg = "xyz";
  const int initial_queue_substr_size = strlen(initial_msg);
  for (int i = 0; i < queue_buffer_size; i++) {
    queue_buffer[i] = initial_msg[i % initial_queue_substr_size];
  }
}

static int helloq_open(devminor_t UNUSED(minor), int UNUSED(access),
                       endpoint_t UNUSED(user_endpt)) {
  return OK;
}

static int helloq_close(devminor_t UNUSED(minor)) {
  return OK;
}

static ssize_t helloq_read(devminor_t UNUSED(minor), u64_t UNUSED(position),
                           endpoint_t endpt, cp_grant_id_t grant, size_t size,
                           int UNUSED(flags), cdev_id_t UNUSED(id)) {
  size = size > queue_buffer_size ? queue_buffer_size : size;
  if (size == 0) {
    return OK;
  }

  int rv = sys_safecopyto(endpt, grant, 0, (vir_bytes)queue_buffer, size);
  if (rv != OK) {
    return rv;
  }

  queue_buffer_size -= size;
  for (int i = 0, j = size; j < queue_buffer_capacity; i++, j++) {
    queue_buffer[i] = queue_buffer[j];
  }

  if (queue_buffer_size * 4 <= queue_buffer_capacity &&
      queue_buffer_capacity > 1) {
    int new_queue_capacity = queue_buffer_capacity / 2;
    resize_queue_buffer(new_queue_capacity);
  }

  /* Return the number of bytes read. */
  return size;
}

static ssize_t helloq_write(devminor_t UNUSED(minor), u64_t UNUSED(pos),
                            endpoint_t ep, cp_grant_id_t grant, size_t size,
                            int UNUSED(flags), cdev_id_t UNUSED(id)) {
  int required_capacity = queue_buffer_size + size;
  int new_capacity = queue_buffer_capacity;
  while (required_capacity > new_capacity) {
    new_capacity *= 2;
  }
  resize_queue_buffer(new_capacity);

  int rv = sys_safecopyfrom(
      ep, grant, 0, (vir_bytes)(queue_buffer + queue_buffer_size), size);
  if (rv != OK) {
    return rv;
  }

  queue_buffer_size += size;
  return size;
}

static int helloq_ioctl(devminor_t UNUSED(minor), unsigned long request,
                        endpoint_t endpt, cp_grant_id_t grant,
                        int UNUSED(flags), endpoint_t user_endpt,
                        cdev_id_t UNUSED(id)) {
  switch (request) {
  case HQIOCRES: {
    // HQIOCRES – restores the queue to the initial state.
    if (queue_buffer_capacity < DEVICE_SIZE) {
      resize_queue_buffer(DEVICE_SIZE);
    }
    queue_buffer_size = DEVICE_SIZE;

    set_initial_queue_value();
    return OK;
  }
  case HQIOCSET: {
    // HQIOCSET – takes char[MSG_SIZE] as an argument and places the string in
    // the queue.

    char buffer[MSG_SIZE];
    int rv = sys_safecopyfrom(endpt, grant, 0, (vir_bytes)buffer, MSG_SIZE);
    if (rv != OK) {
      return rv;
    }

    if (queue_buffer_size >= MSG_SIZE) {
      int offset = queue_buffer_size - MSG_SIZE;
      memcpy(queue_buffer + offset, buffer, MSG_SIZE);
    } else {
      int new_capacity = queue_buffer_capacity;
      while (new_capacity < MSG_SIZE) {
        new_capacity *= 2;
      }
      if (new_capacity != queue_buffer_capacity) {
        resize_queue_buffer(new_capacity);
      }

      memcpy(queue_buffer, buffer, MSG_SIZE);
      queue_buffer_size = MSG_SIZE;
    }

    return OK;
  }
  case HQIOCXCH: {
    // HQIOCXCH – takes char[2] as argument and replaces all char[0] to char[1]
    // in the queue buffer.
    char tab[2];
    int rc = sys_safecopyfrom(endpt, grant, 0, (vir_bytes)tab, 2);
    if (rc == OK) {
      for (int i = 0; i < queue_buffer_size; i++) {
        if (queue_buffer[i] == tab[0]) {
          queue_buffer[i] = tab[1];
        }
      }
    }
    return rc;
  }
  case HQIOCDEL: {
    // HQIOCDEL – removes every third element from the queue. Does not change
    // the capacity of the buffer.
    int i = 0, j = 0;
    while (j < queue_buffer_size) {
      if (j % 3 == 2) {
        j++;
      } else {
        queue_buffer[i] = queue_buffer[j];
        j++;
        i++;
      }
    }
    queue_buffer_size = i;
    return OK;
  }
  default: {
    return ENOTTY;
  }
  }
}

static int sef_cb_lu_state_save(int UNUSED(state)) {
  /* Save the state. */
  ds_publish_u32("queue_buffer_capacity", queue_buffer_capacity, DSF_OVERWRITE);
  ds_publish_u32("queue_buffer_size", queue_buffer_size, DSF_OVERWRITE);
  if (queue_buffer_size > 0) {
    ds_publish_mem("queue_buffer", queue_buffer, queue_buffer_size,
                   DSF_OVERWRITE);
  }
  free(queue_buffer);

  return OK;
}

static int lu_state_restore() {
  /* Restore the state. */
  u32_t value;

  ds_retrieve_u32("queue_buffer_capacity", &value);
  ds_delete_u32("queue_buffer_capacity");
  queue_buffer_capacity = (int)value;
  ds_retrieve_u32("queue_buffer_size", &value);
  ds_delete_u32("queue_buffer_size");
  queue_buffer_size = (int)value;

  queue_buffer = malloc(queue_buffer_capacity);
  if (queue_buffer == NULL) {
    printf("Memory allocation failed in hello queue driver\n");
    exit(1);
  }
  if (queue_buffer_size > 0) {
    size_t size_to_cpy = queue_buffer_size;
    ds_retrieve_mem("queue_buffer", queue_buffer, &size_to_cpy);
    ds_delete_mem("queue_buffer");
  }

  return OK;
}

static void sef_local_startup() {
  /* Register init callbacks. Use the same function for all event types. */
  sef_setcb_init_fresh(sef_cb_init);
  sef_setcb_init_lu(sef_cb_init);
  sef_setcb_init_restart(sef_cb_init);

  /* Register live update callbacks. */
  /* - Agree to update immediately when LU is requested in a valid state. */
  sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
  /* - Support live update starting from any standard state. */
  sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
  /* - Register a custom routine to save the state. */
  sef_setcb_lu_state_save(sef_cb_lu_state_save);

  /* Let SEF perform startup. */
  sef_startup();
}

static int sef_cb_init(int type, sef_init_info_t *UNUSED(info)) {
  /* Initialize the hello driver. */
  int do_announce_driver = TRUE;

  switch (type) {
  case SEF_INIT_FRESH: {
    queue_buffer_capacity = DEVICE_SIZE;
    queue_buffer_size = DEVICE_SIZE;
    queue_buffer = malloc(DEVICE_SIZE);
    if (queue_buffer == NULL) {
      printf("Memory allocation failed in hello queue driver\n");
      exit(1);
    }

    set_initial_queue_value();
    break;
  }
  case SEF_INIT_LU: {
    lu_state_restore();
    do_announce_driver = FALSE;
    break;
  }
  case SEF_INIT_RESTART: {
    lu_state_restore();
    break;
  }
  }

  /* Announce we are up when necessary. */
  if (do_announce_driver) {
    chardriver_announce();
  }

  /* Initialization completed successfully. */
  return OK;
}

int main(void) {
  /* Perform initialization. */
  sef_local_startup();

  /* Run the main loop. */
  chardriver_task(&helloq_tab);

  free(queue_buffer);
  return OK;
}
