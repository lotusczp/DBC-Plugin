#ifndef DBCPLUGIN_H
#define DBCPLUGIN_H

#include "dbcplugin_global.h"

#include "LTransPluginFactory.h"

class DBCPLUGINSHARED_EXPORT DbcPlugin : public LTransPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "Lobster.LTransPluginFactory" FILE "DbcPlugin.json")
    Q_INTERFACES(LTransPluginFactory)

public:
    DbcPlugin() {}
    ~DbcPlugin() Q_DECL_OVERRIDE {}

    //! create the transmission instance
    virtual LTransmission* createTransInstance() Q_DECL_OVERRIDE;
};

#endif // DBCPLUGIN_H
