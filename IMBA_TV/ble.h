#pragma once
#include "euc_data.h"

// update_mode: режим обновления по Bluetooth — вместо сервиса телеметрии
// работает сервис DFU, через который сайт или nRF Connect перезагружают плату в загрузчик
void bleInit(bool update_mode = false);
void bleAdvertise(bool update_mode = false);
euc_data_t& bleGetEucData();
