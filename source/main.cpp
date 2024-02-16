#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <borealis.hpp>

using namespace std;
using namespace brls::i18n::literals;

#define NS_APPLICATION_RECORD_SIZE 100
#define SECONDS_PER_HOUR 3600
#define NANOSECONDS_PER_SECOND 1000000000
#define SECONDS_PER_MINUTE 60

int main(int argc, char *argv[]) {
    if (!brls::Application::init(APP_TITLE)
        || R_FAILED(nsInitialize())
        || R_FAILED(pdmqryInitialize())
        || R_FAILED(accountInitialize(AccountServiceType_Administrator))
    ) {
        return EXIT_FAILURE;
    }

    brls::i18n::loadTranslations();

    romfsInit();
    nsInitialize();

    brls::AppletFrame *rootFrame = new brls::AppletFrame(false, false);
    rootFrame->setIcon(BOREALIS_ASSET("icon/icon.jpg"));
    rootFrame->setTitle(std::string(APP_TITLE));
    rootFrame->setFooterText("v" + std::string(APP_VERSION));

    AccountUid accountUid;
    AccountProfile profile;
    AccountProfileBase profileBase = {};
    PselUserSelectionSettings pselUserSelectionSettings;
    NsApplicationRecord nsApplicationRecord[NS_APPLICATION_RECORD_SIZE] = {0};
    PdmPlayStatistics pdmPlayStatistics[1] = {0};
    NsApplicationControlData nsApplicationControlData;
    NacpLanguageEntry *nacpLanguageEntry = NULL;

    s32 outEntryCount = -1;
    size_t actualSize = -1;
    u8* iconBuffer;
    u32 imageSize;
    char timeTextBuffer[100];

    memset(&nsApplicationControlData, 0, sizeof(nsApplicationControlData));
    memset(&accountUid, 0, sizeof(accountUid));
    memset(&pselUserSelectionSettings, 0, sizeof(pselUserSelectionSettings));

    auto applicationList = new brls::List();

    if (R_SUCCEEDED(pselShowUserSelector(&accountUid, &pselUserSelectionSettings))
        && R_SUCCEEDED(accountGetProfile(&profile, accountUid)) && R_SUCCEEDED(accountProfileGet(&profile, nullptr, &profileBase))
        && R_SUCCEEDED(nsListApplicationRecord(nsApplicationRecord, NS_APPLICATION_RECORD_SIZE, 0, &outEntryCount))
    ) {
        if (R_SUCCEEDED(accountProfileGetImageSize(&profile, &imageSize))
            && (iconBuffer = (u8*)malloc(imageSize)) != NULL && R_SUCCEEDED(accountProfileLoadImage(&profile, iconBuffer, imageSize, &imageSize))
        ) {
            rootFrame->setIcon(iconBuffer, imageSize);
        }

        rootFrame->setTitle(std::string(profileBase.nickname, 0x20));

        for (int i = 0; i < outEntryCount; i++) {
            if (R_SUCCEEDED(pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(nsApplicationRecord[i].application_id, accountUid, false, pdmPlayStatistics))
                && R_SUCCEEDED(nsGetApplicationControlData(NsApplicationControlSource_Storage, nsApplicationRecord[i].application_id, &nsApplicationControlData, sizeof(NsApplicationControlData), &actualSize))
                && R_SUCCEEDED(nacpGetLanguageEntry(&nsApplicationControlData.nacp, &nacpLanguageEntry))
            ) {
                u64 playtimeSeconds = pdmPlayStatistics->playtime / NANOSECONDS_PER_SECOND;
                u64 playtimeHours = playtimeSeconds / SECONDS_PER_HOUR;
                playtimeSeconds -= playtimeHours * SECONDS_PER_HOUR;
                u64 playtimeMinutes = playtimeSeconds / SECONDS_PER_MINUTE;
                playtimeSeconds -= playtimeMinutes * SECONDS_PER_MINUTE;

                sprintf(timeTextBuffer, "%02ld:%02ld:%02ld", playtimeHours, playtimeMinutes, playtimeSeconds);

                auto listItem = new brls::ListItem(std::string(nacpLanguageEntry->name), "", timeTextBuffer);

                if (sizeof(nsApplicationControlData.icon)) {
                    listItem->setThumbnail(nsApplicationControlData.icon, sizeof(nsApplicationControlData.icon));
                }

                applicationList->addView(listItem);
            }
        }
    } else {
        brls::Application::quit();
    }

    rootFrame->setContentView(applicationList);

    rootFrame->registerAction("brls/hints/back"_i18n, brls::Key::B, [rootFrame] {
        brls::Application::quit();
        return true;
    });

    brls::Application::pushView(rootFrame);

    while (brls::Application::mainLoop());

    nsExit();
    pdmqryExit();
    accountExit();

    return EXIT_SUCCESS;
}