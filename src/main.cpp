#include "main.h"
#include "xxhash.h"

#include "iplugingame.h"
#include "ipluginlist.h"
#include "ifiletree.h"
#include "pluginsetting.h"
#include "utility.h"

#include <unordered_set>
#include <iostream>
#include <fstream>

#include <QApplication>
#include <QMessageBox>
#include <questionboxmemory.h>

#include <Qt>                    // for Qt::CaseInsensitive
#include <QtDebug>               // for qCritical, qDebug

using namespace MOBase;

SanityCheck::SanityCheck() : m_MOInfo(nullptr) {
    connect(this, &SanityCheck::showMemoryMessageBox, this, &SanityCheck::onShowMemoryMessageBox);
}

SanityCheck::~SanityCheck() {
}

bool SanityCheck::init(IOrganizer* moInfo)
{

    m_MOInfo = moInfo;

    if (!m_MOInfo->modList()->onModInstalled(
        [&](const MOBase::IModInterface* mods) {
            singleDotOverrideCheck(mods);
        })) {
        qCritical("failed to execute callback onModInstalled");
        return false;
    }

    if (!m_MOInfo->onUserInterfaceInitialized([&](QMainWindow* mainWindow) {
        firstTimeSetupNoticeNewVegas();
        fnvLangESPCheck();
        executableHashCheck();
        geckIniCheck();
        dotOverrideInitCheck();
        vortexFilesCheck();
        })) {
        qCritical("failed to execute callback onUserInterfaceInitialized");
        return false;
    }

    if (!m_MOInfo->onAboutToRun([&](const auto& binary) {
        bethsPluginCheck();
        return true;
        })) {
        qCritical("failed to execute callback onAboutToRun");
        return false;
    }
    /*
    if (!m_MOInfo->onNextRefresh([&]() {
        
        }, false)) {
        qCritical("failed to execute callback onNextRefresh");
        return false;
    }
    */

    return true;
}

QString SanityCheck::name() const
{
    return "Sanity Check";
}

QString SanityCheck::localizedName() const
{
    return tr("Sanity Check");
}

QString SanityCheck::author() const
{
    return "Senjay";
}

QString SanityCheck::description() const
{
    return tr("Handles common problems");
}

VersionInfo SanityCheck::version() const
{
    return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

/* Uncomment if you want this plugin to run on specific game
std::vector<std::shared_ptr<const MOBase::IPluginRequirement>> SanityCheck::requirements() const
{
  return { Requirements::gameDependency("TTW"),
           Requirements::gameDependency("New Vegas"),
           Requirements::gameDependency("Skyrim Special Edition"),
           Requirements::gameDependency("Fallout 4")
  };
}
*/

QList<PluginSetting> SanityCheck::settings() const
{
    return QList<PluginSetting>()
        << PluginSetting("dotoverrideinit_check", tr("Automatic .override creation for all mods at initialization in FNV and TTW instances (requires JIP LN to have any effect)."), true)
        << PluginSetting("singledotoverride_check", tr("Automatic .override creation when installing a mod in FNV and TTW instances (requires JIP LN to have any effect)."), true)
        << PluginSetting("fnvlang_check", tr("Show a prompt when the translation plugin is present in the game directory"), true)
        << PluginSetting("executablehash_check", tr("Show a prompt when FNV game executable is unpatched"), true)
        << PluginSetting("geckini_check", tr("Automatic addition of optimal geck configuration"), true)
        << PluginSetting("bethsplugin_check", tr("Show a prompt when plugins/light plugins have exceeded the limit"), true)
        << PluginSetting("firsttimesetupnv_check", tr("Show a first time setup notice when managing a new FNV/TTW instance"), true)
        << PluginSetting("vortexfiles_check", tr("Scan the game directory for vortex files"), true)
        ;
}

uint64_t calculateFileHash(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    const size_t bufferSize = static_cast<size_t>(64) * 1024;
    std::vector<char> buffer(bufferSize);
    XXH64_state_t* state = XXH64_createState();
    if (state == nullptr) {
        throw std::runtime_error("Failed to create xxHash state");
    }
    XXH64_reset(state, 0);
    while (file.good()) {
        file.read(buffer.data(), bufferSize);
        size_t bytesRead = file.gcount();
        if (bytesRead > 0) {
            XXH64_update(state, buffer.data(), bytesRead);
        }
    }
    uint64_t hash = XXH64_digest(state);
    XXH64_freeState(state);
    return hash;
}

QString getFileNameWithoutExtension(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    return fileInfo.completeBaseName();
}

bool fileExists(const QString& filePath) {
    return QFile::exists(filePath);
}

void SanityCheck::singleDotOverrideCheck(const MOBase::IModInterface* mod) const
{
    if (!m_MOInfo->pluginSetting(name(), "singledotoverride_check").toBool())
        return;

    const auto& managedGame = m_MOInfo->managedGame()->gameName();
    if (managedGame == "TTW" || managedGame == "New Vegas")
    {
        const auto modPath = mod->absolutePath();
        QList<QString> fileNames;
        QDir dir(modPath);
        QStringList bsaFiles = dir.entryList(QStringList() << "*.bsa", QDir::Files | QDir::NoDotAndDotDot);

        for (const QString& file : bsaFiles) {
            QFileInfo fileInfo(file);
            fileNames.append(fileInfo.baseName()); // Get filenames without extension
        }

        for (const QString& fileName : fileNames) {
            QString overrideFileName = fileName + ".override";
            QString overrideFilePath = dir.filePath(overrideFileName);

            if (!QFile::exists(overrideFilePath)) {
                // Create a dummy .override file
                QFile overrideFile(overrideFilePath);
                if (overrideFile.open(QIODevice::WriteOnly)) {
                    overrideFile.close(); // Close it immediately after creating
                    qInfo() << "An override file for " << fileName << " is missing from " << mod->name() << ". MO2 will automatically create one.";
                }
                else {
                    qWarning() << "Failed to write a dummy override file for " << fileName;
                }
            }
        }
    }
}

void SanityCheck::dotOverrideInitCheck() const
{
    if (!m_MOInfo->pluginSetting(name(), "dotoverrideinit_check").toBool())
        return;
    TimeThis tt("SanityCheck::dotOverrideInitCheck()");

    const auto& managedGame = m_MOInfo->managedGame()->gameName();

    if (managedGame == "TTW" || managedGame == "New Vegas")
    {
        const auto modPath = m_MOInfo->modsPath();
        QDir modDirs(modPath);
        QStringList modDirectories =
            modDirs.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString& modDir : modDirectories) {
            if (modDir == ".git") {
                continue;  // Ignore .git directory
            }

            QDir modDirPath = modDirs.absoluteFilePath(modDir);
            QStringList bsaFiles =
                modDirPath.entryList(QStringList() << "*.bsa", QDir::Files);
            if (!bsaFiles.isEmpty()) {
                QString filename = bsaFiles.first();

                QString bsaNameWithOverride =
                    modPath + "\\" + modDir + "\\" +
                    getFileNameWithoutExtension(filename) + ".override";

                if (!fileExists(bsaNameWithOverride)) {
                    qInfo() << "An override file for " << filename << " is missing, MO2 will automatically create one.";
                    QFile createOverrideFileFromBSA(bsaNameWithOverride);
                    if (createOverrideFileFromBSA.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        createOverrideFileFromBSA.close();
                    }
                }
            }
        }
    }
}

void SanityCheck::fnvLangESPCheck() const
{
    if (!m_MOInfo->pluginSetting(name(), "fnvlang_check").toBool())
        return;
    const auto& managedGame = m_MOInfo->managedGame()->gameName();
    if (managedGame == "TTW" || managedGame == "New Vegas")
    {
        const QString& langFilePath(m_MOInfo->managedGame()->dataDirectory().absoluteFilePath("FalloutNV_lang.esp"));
        if (fileExists(langFilePath)) {
            qWarning("Translation plugin detected, open the notification for more information.");
            /*
            const auto& response = QuestionBoxMemory::query(
                nullptr, "fnvLangESPCheck",
                tr("FalloutNV_lang.esp was found"),
                tr("This translation plugin directly edits thousands of records to change the language, which will cause many incompatibilities with most mods.\n\nDo you want to delete it?"),
                QDialogButtonBox::Yes | QDialogButtonBox::No,
                QDialogButtonBox::No);

            switch (response) {
            case QMessageBox::Yes:
                shellDeleteQuiet(langFilePath);
                m_MOInfo->refresh();
                qInfo("FalloutNV_lang.esp has been deleted from the game directory");
                return;
                break;
            case QMessageBox::No:
                return;
                break;
            }
            */
        }
    }
}

bool SanityCheck::fnvLangESPCheckAlt() const
{
    if (!m_MOInfo->pluginSetting(name(), "fnvlang_check").toBool())
        return false;
    const auto& managedGame = m_MOInfo->managedGame()->gameName();
    if (managedGame == "TTW" || managedGame == "New Vegas") {
        const QString& langFilePath(
            m_MOInfo->managedGame()->dataDirectory().absoluteFilePath("FalloutNV_lang.esp"));
        if (fileExists(langFilePath)) {
            //qWarning("Translation plugin detected, open the notification for more information.");
            return true;
        }
        return false;
    }
    return false;
}

void SanityCheck::executableHashCheck() const
{
    if (!m_MOInfo->pluginSetting(name(), "executablehash_check").toBool())
        return;
    const auto& managedGame = m_MOInfo->managedGame()->gameName();
    if (managedGame == "TTW" || managedGame == "New Vegas")
    {
        uint64_t executableHash =
            calculateFileHash(m_MOInfo->managedGame()->gameDirectory().absoluteFilePath(
                m_MOInfo->managedGame()->binaryName()).toStdString());

        const std::unordered_set<uint64_t> patchedExecutableHashes = {
            7625907240992332651,   // patched gog
            7658157216307907036,   // patched epic
            12047903279041789040,  // patched steam
        };

        if (patchedExecutableHashes.count(executableHash) > 0) {
            return;
        }
        qWarning("Unpatched game executable detected, open the notification for more information");
        /*
        emit showMemoryMessageBox(
            "executableHashCheck", "Unpatched game executable",
            tr("The game executable hasn't been patched with the 4GB Patcher. It won't "
                "load xNVSE and will be limited to 2GB of RAM<br><br>You can download "
                "and install the "
                "patch from the link "
                "below:<br><a "
                "href=\"https://www.nexusmods.com/newvegas/mods/62552\">https://"
                "www.nexusmods.com/newvegas/mods/62552</a><br><br>Epic version "
                "below:<br><a "
                "href=\"https://www.nexusmods.com/newvegas/mods/81281\">https://"
                "www.nexusmods.com/newvegas/mods/81281</a>"),
            QDialogButtonBox::Ok, QDialogButtonBox::Ok);
        */
    }
    return;
}

bool SanityCheck::executableHashCheckAlt() const
{
    if (!m_MOInfo->pluginSetting(name(), "executablehash_check").toBool())
        return false;
    const auto& managedGame = m_MOInfo->managedGame()->gameName();
    if (managedGame == "TTW" || managedGame == "New Vegas") {
        const auto gameBinary = m_MOInfo->managedGame()->binaryName();
        uint64_t executableHash = calculateFileHash(
            m_MOInfo->managedGame()->gameDirectory().absoluteFilePath(gameBinary).toStdString());

        const std::unordered_set<uint64_t> patchedExecutableHashes = {
            7625907240992332651,   // patched gog
            7658157216307907036,   // patched epic
            12047903279041789040,  // patched steam
        };

        if (patchedExecutableHashes.count(executableHash) > 0) {
            return false;
        }
        //qWarning("Unpatched game executable detected, open the notification for more information");
        return true;
    }
    return false;
}

void SanityCheck::bethsPluginCheck() const
{
    if (!m_MOInfo->pluginSetting(name(), "bethsplugin_check").toBool())
        return;

    const auto pluginList = m_MOInfo->pluginList();
    QStringList names = pluginList->pluginNames();
    QStringList activePlugins;
    for (auto& name : names) {
        if (pluginList->state(name) == IPluginList::STATE_ACTIVE) {
            activePlugins.append(name);
        }
    }

    int espCount = 0;
    int esmCount = 0;
    int eslCount = 0;

    for (const auto& activePlugin : activePlugins) {
        if (activePlugin.endsWith(".esp")) {
            pluginList->isLightFlagged(activePlugin) ? eslCount++ : espCount++;
        }
        else if (activePlugin.endsWith(".esm")) {
            pluginList->isLightFlagged(activePlugin) ? eslCount++ : esmCount++;
        }
        else if (activePlugin.endsWith(".esl")) {
            eslCount++;
        }
    }

    if (espCount + esmCount > 254) {
        QuestionBoxMemory::query(
            QApplication::activeModalWidget(), "bethsPluginCheck",
            tr("Active plugin exceeds the limit"),
            tr("You have more than 254 active plugins which exceeds the limit that the game can handle.\n\nPlugins past 254 will not load"),
            QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
        return;
    }
    else if (eslCount > 4096) {
        QuestionBoxMemory::query(
            QApplication::activeModalWidget(), "bethsLightPluginCheck",
            tr("Active light plugin exceeds the limit"),
            tr("You have more than 4096 active light plugins (esl) which exceeds the limit that the game can handle.\n\nLight plugins past 4096 will not load"),
            QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
        return;
    }
}

void SanityCheck::geckIniCheck() const
{
    const auto managedGame = m_MOInfo->managedGame()->gameName();
    if (managedGame == "TTW" || managedGame == "New Vegas")
    {
        QString geckIniPath = m_MOInfo->profilePath() + "\\GECKCustom.ini";
        std::ifstream file(geckIniPath.toStdString());

        if (file.peek() == std::ifstream::traits_type::eof()) { //Check if the file is empty
            file.close();
            std::ofstream outfile(geckIniPath.toStdString());
            qInfo() << "Writing optimal configuration to GECKCustom.ini";
            if (outfile.is_open()) {
                outfile << "[General]\n";
                outfile << "bUseMultibounds=0\n";
                outfile << "bAllowMultipleMasterLoads=1\n";
                outfile << "bAllowMultipleEditors=1\n";
                outfile << "[Localization]\n";
                outfile << "iExtendedTopicLength=255\n";
                outfile << "bAllowExtendedText=1\n";
                outfile.close();
            }
            else {
                qWarning() << "Error opening " << geckIniPath << " for writing.";
            }
        }
    }
}

void SanityCheck::firstTimeSetupNoticeNewVegas() const
{
    if (!m_MOInfo->pluginSetting(name(), "firsttimesetupnv_check").toBool())
        return;
    const auto managedGame = m_MOInfo->managedGame()->gameName();
    if (managedGame == "TTW" || managedGame == "New Vegas") {
        qInfo() << "MO2 has detected that this is your first time managing the game, "
            "open the notification for an important notice";
    }
}

bool SanityCheck::firstTimeSetupNoticeNewVegasAlt() const
{
    if (!m_MOInfo->pluginSetting(name(), "firsttimesetupnv_check").toBool())
        return false;
    const auto managedGame = m_MOInfo->managedGame()->gameName();
    if (managedGame == "TTW" || managedGame == "New Vegas") {
        return true;
    }
    return false;
}

void SanityCheck::vortexFilesCheck() const
{
    if (!m_MOInfo->pluginSetting(name(), "vortexfiles_check").toBool())
        return;

    const QDir& gameDirectory = m_MOInfo->managedGame()->gameDirectory();
    std::unordered_set<std::string> vortexFiles = {
        "__folder_managed_by_vortex",
        "vortex.deployment.json"
    };

    QStringList foundFiles = gameDirectory.entryList(QDir::Files | QDir::Dirs, QDir::Name);
    for (const auto& file : foundFiles) {
        if (vortexFiles.find(file.toStdString()) != vortexFiles.end()) {
            
            qWarning() << "Dirty game files detected, open the notification for more information";

            return;
        }
    }
}

bool SanityCheck::vortexFilesCheckAlt() const
{
    if (!m_MOInfo->pluginSetting(name(), "vortexfiles_check").toBool())
        return false;

    const QDir& gameDirectory = m_MOInfo->managedGame()->gameDirectory();
    std::unordered_set<std::string> vortexFiles = {
        "__folder_managed_by_vortex",
        "vortex.deployment.json"
    };

    QStringList foundFiles = gameDirectory.entryList(QDir::Files | QDir::Dirs, QDir::Name);
    for (const auto& file : foundFiles) {
        if (vortexFiles.find(file.toStdString()) != vortexFiles.end()) 
            return true;
    }
    return false;
}

void SanityCheck::onShowMemoryMessageBox(const QString& memory, const QString& title, const QString& text,
    const QDialogButtonBox::StandardButtons yesandnobutton,
    const QDialogButtonBox::StandardButton defaultbutton)
{
    const auto& response = QuestionBoxMemory::query(
        nullptr, memory, title, text, yesandnobutton, defaultbutton);
}

std::vector<unsigned int> SanityCheck::activeProblems() const
{
    std::vector<unsigned int> result;

    if (m_MOInfo->pluginSetting(name(), "firsttimesetupnv_check").toBool() && firstTimeSetupNoticeNewVegasAlt())
    {
        result.push_back(PROBLEM_FIRSTTIME_SETUPNV);
    }
    if (m_MOInfo->pluginSetting(name(), "fnvlang_check").toBool() && fnvLangESPCheckAlt())
    {
        result.push_back(PROBLEM_FNVLANG);
    }
    if (m_MOInfo->pluginSetting(name(), "executablehash_check").toBool() &&
        executableHashCheckAlt()) {
        result.push_back(PROBLEM_UNPATCHED_EXE);
    }
    if (m_MOInfo->pluginSetting(name(), "vortexfiles_check").toBool() &&
        vortexFilesCheckAlt()) {
        result.push_back(PROBLEM_DIRTYFILES);
    }

    //if (result.size() > 0)
        //qInfo() << "There's unresolved problems in the notification";
 
    return result;
}

QString SanityCheck::shortDescription(unsigned int key) const
{
    switch (key) {
    case PROBLEM_FIRSTTIME_SETUPNV:
        return tr("First time setup important notice");
        break;
    case PROBLEM_FNVLANG:
        return tr("Translation plugin present in the game directory");
        break;
    case PROBLEM_UNPATCHED_EXE:
        return tr("Unpatched game executable");
        break;
    case PROBLEM_DIRTYFILES:
        return tr("Dirty files in the game folder");
        break;
    default:
        throw MyException(tr("invalid problem key %1").arg(key));
    }
}

QString SanityCheck::fullDescription(unsigned int key) const
{
    switch (key) {
    case PROBLEM_FIRSTTIME_SETUPNV:
        return tr(
            "1. <b>Base Address Randomization</b><br>"
            "Base Address Randomization is a security feature in Windows that allows "
            "program's starting address to be randomized, which will crash the game "
            "when using NVSE plugins or the 4GB Patch.<br><br>"
            "Please check that it's disabled in the instruction below:"
            "<ol>"
            "<li> Open <b>Windows Security</b> from your Start Menu.</li>"
            "<li> Click on <b>App & browser control</b> in the left sidebar.</li>"
            "<li> Click on <b>Exploit protection settings</b> under <b>Exploit protection</b>.</li>"
            "<li> Ensure <b>Force randomization for images(Mandatory ASLR)</b> is set to <b>Use default (Off)</b>.</li>"
            "</ol>"
            "2. <b>Outdated AMD GPU Driver Crash</b> (only applies to <b>AMD GPU Users</b>)<br>"
            "GPU driver version from <b>24.1.1</b> up to <b>24.5.1</b> may fail to "
            "compile the shader and <b>crash the game</b>. The issue is stated "
            "on the official AMD website in this <a href='https://www.amd.com/en/resources/support-articles/release-notes/RN-RAD-WIN-24-4-1.html'>link</a>.<br>"
            "Make sure that your driver version is updated.<br><br>"
            "<b>If you've acknowledged the notice, press Fix to resolve it</b>."
        );
        break;
    case PROBLEM_FNVLANG:
        return tr(
            "MO2 has detected that there's a translation plugin named <b>FalloutNV_lang.esp</b> "
            "in the game files.<br><br>This translation plugin is bundled with the base game and it "
            "directly edits thousands of records to change the language, which will "
            "cause many incompatibilities with most mods.<br><br>"
            "Pressing fix will delete the file");
        break;
    case PROBLEM_UNPATCHED_EXE:
        return tr(
            "MO2 has detected that the game executable hasn't been patched with "
            "the 4GB Patcher.<br>It won't load xNVSE and will be limited to 2GB of RAM<br><br>"
            "You can download and install the patch from the link below:<br><a "
            "href=\"https://www.nexusmods.com/newvegas/mods/62552\">https://"
            "www.nexusmods.com/newvegas/mods/62552</a><br><br>Epic version "
            "below:<br><a "
            "href=\"https://www.nexusmods.com/newvegas/mods/81281\">https://"
            "www.nexusmods.com/newvegas/mods/81281</a>");
        break;
    case PROBLEM_DIRTYFILES:
        return tr(
            "MO2 has detected that there are residual vortex files inside the game folder.<br><br>"
            "It's advised to do a clean deletion of the game files and reinstall before "
            "managing mods with MO2.<br>Below are the instruction for doing a clean deletion "
            "of the game files according to your platform:<br><br>"
            "<b>Steam</b>"
            "<ol>"
            "<li>Open <b>Steam</b>, go to your <b>Library</b> and find <b>Fallout: New Vegas</b></li>"
            "<li>Right click and choose <b>Properties</b></li>"
            "<li>Click the <b>Installed Files</b> then <b>Browse</b></li>"
            "<li>Delete all the game files</li>"
            "<li>Enter this path in your file explorer <b>%USERPROFILE%\\Documents\\My Games\\FalloutNV</b> then delete all the .ini files</li>"
            "</ol>"
            "<b>GOG</b>"
            "<ol>"
            "<li>Navigate to where you've installed Fallout: New Vegas</li>"
            "<li>Delete all the game files</li>"
            "<li>Enter this path in your file explorer <b>%USERPROFILE%\\Documents\\My Games\\FalloutNV</b> then delete all the .ini files</li>"
            "</ol>"
        );
        break;
    default:
        throw MyException(tr("invalid problem key %1").arg(key));
    }
}

bool SanityCheck::hasGuidedFix(unsigned int key) const
{
    switch (key)
    {
    case PROBLEM_FIRSTTIME_SETUPNV:
        return true;
        break;
    case PROBLEM_FNVLANG:
        return true;
        break;
    case PROBLEM_UNPATCHED_EXE:
        return false;
        break;
    case PROBLEM_DIRTYFILES:
        return false;
        break;
    default:
        throw MyException(tr("invalid problem key %1").arg(key));
    }
}

void SanityCheck::startGuidedFix(unsigned int key) const
{
    switch (key)
    {
    case PROBLEM_FIRSTTIME_SETUPNV:
        m_MOInfo->setPluginSetting(name(), "firsttimesetupnv_check", false);
        break;
    case PROBLEM_FNVLANG:
        shellDeleteQuiet(m_MOInfo->managedGame()->dataDirectory().absoluteFilePath("FalloutNV_lang.esp"));
        m_MOInfo->refresh();
        qInfo("FalloutNV_lang.esp has been deleted from the game directory");
        break;
    default:
        throw MyException(tr("invalid problem key %1").arg(key));
    }
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
Q_EXPORT_PLUGIN2(SanityCheck, SanityCheck)
#endif