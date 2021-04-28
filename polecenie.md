Do dodania:

wywołanie systemowe `PM_NEGATEEXIT` 

funkcja biblioteczna `int negateexit(int negate)` w pliku `unistd.h`.

------------------------------------------------------------------

Negacja kodu powrotu procesu

W MINIX-ie proces kończy działanie wywołując `_exit(status)`. Rodzic może odczytać kod powrotu swojego potomka, korzystając np. z `wait`.
Chcemy umożliwić procesowi wpływanie na wartość kodu powrotu swojego i swoich nowo tworzonych dzieci.

------------------------------------------------------------------

`int negateexit(int negate)`

`negate != 0` -> powoduje, że gdy proces wywołujący tę funkcję zakończy działanie z kodem zero, rodzic odczyta kod powrotu równy jeden, a gdy zakończy działanie z kodem różnym od zera – rodzic odczyta zero

`negate == 0` -> przywraca standardową obsługę kodów powrotu.

retvalue - Wartość zwracana przez tę funkcję to informacja o zachowaniu procesu przed wywołaniem funkcji: 

`0`  -> kody powrotu nie były zmieniane

`1`  –> były negowane. 

`-1` -> jeśli wystąpi jakiś błąd, należy też ustawić errno na odpowiednią wartość.

------------------------------------------------------------------

Nowo tworzony proces ma dziedziczyć aktualne zachowanie rodzica

Kod powinien być zmieniany tylko jeżeli proces kończy się używając syscalla `PM_EXIT` wołanego z `_exit()`

dafaultowo nie są negowane

Działanie funkcji `negateexit()` powinno polegać na użyciu nowego wywołania systemowego `PM_NEGATEEXIT`, które należy dodać do serwera PM. Do przekazania parametru należy zdefiniować własny typ komunikatu.
