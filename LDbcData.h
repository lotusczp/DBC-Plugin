#ifndef LDBCDATA_H
#define LDBCDATA_H

#include <QString>
#include <QMap>
#include "LDbcManagerDllWrapper.h"

struct LDbcData
{
    DBCSignal   signal;

    bool setValue(double a_value)
    {
        if(a_value > signal.nMax) a_value = signal.nMax;
        else if(a_value < signal.nMin) a_value = signal.nMin;

        // Calculate the raw bytes anyway
        if(!signal.is_signed) {
            signal.nRawValue = (uint64_t)((a_value-signal.nOffset)/signal.nFactor);
        }
        else {
            int64_t tmp = (int64_t)((a_value-signal.nOffset)/signal.nFactor);
            if(tmp < 0) {
                tmp &= ((1ULL << signal.nLen) - 1);
            }
            signal.nRawValue = (uint64_t)tmp;
        }

        if(a_value != signal.nValue) {
            signal.nValue = a_value;
            return true;
        }

        return false;
    }

    bool setRawValue(quint64 rawValue) {
        double value = rawValue*signal.nFactor+signal.nOffset;

        if(value > signal.nMax) value = signal.nMax;
        else if(value < signal.nMin) value = signal.nMin;

        if(value != signal.nValue) {
            signal.nValue = value;
            return true;
        }

        return false;

    }

    void initRawValue()
    {
        signal.nRawValue = (uint64_t)((signal.nValue-signal.nOffset)/signal.nFactor);
    }
};

typedef QMap<QString, LDbcData*>    LDbcDataMap;
typedef QMap<QString, DBCMessage*>  LDbcMsgNameMap;
typedef QMap<quint32, DBCMessage*>  LDbcMsgIdMap;

#endif // LDBCDATA_H
