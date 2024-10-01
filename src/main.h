#pragma once

#include "iplugin.h"
#include "iplugintool.h"
#include "imodinterface.h"
#include "iplugindiagnose.h"
#include <QObject>
#include <QDialogButtonBox>

enum SanityCheckEnum {
    PROBLEM_FIRSTTIME_SETUPNV,
    PROBLEM_FNVLANG,
    PROBLEM_UNPATCHED_EXE,
    PROBLEM_BETHSPLUGIN,
    PROBLEM_DIRTYFILES
};

namespace MOBase {
    struct PluginSetting;
}

class SanityCheck :  public MOBase::IPluginTool, public MOBase::IPluginDiagnose
{

    Q_OBJECT
    Q_INTERFACES(MOBase::IPlugin MOBase::IPluginTool MOBase::IPluginDiagnose)
        
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        Q_PLUGIN_METADATA(IID "senjay.mo2.SanityCheck")
#endif

public:
    SanityCheck();
    ~SanityCheck();

public: // IPlugin
    virtual bool init(MOBase::IOrganizer* moInfo) override;
    virtual QString name() const override;
    virtual QString localizedName() const override;
    //virtual std::vector<std::shared_ptr<const MOBase::IPluginRequirement>> requirements() const override;
    virtual QString author() const override;
    virtual QString description() const override;
    virtual MOBase::VersionInfo version() const override;
    virtual QList<MOBase::PluginSetting> settings() const override;

public: // IPluginTool interface
    virtual QString displayName() const override;
    virtual QString tooltip() const override;
    virtual QIcon icon() const override;

public:  // IPluginDiagnose interface
    virtual std::vector<unsigned int> activeProblems() const override;
    virtual QString shortDescription(unsigned int key) const override;
    virtual QString fullDescription(unsigned int key) const override;
    virtual bool hasGuidedFix(unsigned int key) const override;
    virtual void startGuidedFix(unsigned int key) const override;


signals:
    void showMemoryMessageBox(const QString& memory, const QString& title,
        const QString& text,
        const QDialogButtonBox::StandardButtons yesandnobutton,
        const QDialogButtonBox::StandardButton defaultbutton) const;

public slots:
    void display() const override;

private slots:
    void onShowMemoryMessageBox(const QString& memory, const QString& title,
        const QString& text,
        const QDialogButtonBox::StandardButtons yesandnobutton,
        const QDialogButtonBox::StandardButton defaultbutton);

private:
    void dotOverrideInitCheck() const;
    void singleDotOverrideCheck(const MOBase::IModInterface* mod) const;
    void fnvLangESPCheck() const;
    bool fnvLangESPCheckAlt() const;
    void bethsPluginCheck() const;
    void executableHashCheck() const;
    bool executableHashCheckAlt() const;
    void geckIniCheck() const;
    void firstTimeSetupNoticeNewVegas() const;
    bool firstTimeSetupNoticeNewVegasAlt() const;
    void vortexFilesCheck() const;
    bool vortexFilesCheckAlt() const;
    uint64_t calculateFileHash(const std::string& filePath) const;
    QString getFileNameWithoutExtension(const QString& filePath) const;
    bool fileExists(const QString& filePath) const;
    void setRegistryValue(const QString& key, const QString& value) const;
    QString getRegistryValue(const QString& key) const;
    void deleteRegistryValue(const QString& key) const;

private:
    MOBase::IOrganizer* m_MOInfo;
};
