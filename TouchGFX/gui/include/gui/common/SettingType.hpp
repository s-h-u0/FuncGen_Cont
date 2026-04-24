#pragma once

enum class SettingType {
    Voltage,

    TripCurrent,
    Current = TripCurrent,   // 互換名: 既存コード移行まで残す

    Phase,
    ID
};
