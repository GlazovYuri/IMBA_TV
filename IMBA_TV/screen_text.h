#pragma once

// Общие приёмы вывода текста для экранов-таблиц (сетка, связь и версии)

// крупный текст (scale 2) с узкой десятичной точкой, прижат к правому краю right (последний видимый столбец)
void screenTextBig(int right, int y, const char* text);

// единица измерения мелким шрифтом справа от крупного числа, выровнена по его низу
void screenTextUnit(int x, int y, const char* unit);
