#include "DbcPlugin.h"


#include "LDbcTrans.h"

LTransmission *DbcPlugin::createTransInstance()
{
    return new LDbcTrans;
}
