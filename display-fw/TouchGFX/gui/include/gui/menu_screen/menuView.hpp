#ifndef MENUVIEW_HPP
#define MENUVIEW_HPP

#include <gui_generated/menu_screen/menuViewBase.hpp>
#include <gui/menu_screen/menuPresenter.hpp>
#include "unitConverter.hpp"
#include "DynamicDrawableListItems.hpp"
#include <string>
#include <vector>
#include "ProfileParser.hpp"
#include "Modal.hpp"
#include "UiErrorState.hpp"
#include "stm32u5xx.h"

#define USER_FLASH_ADDRESS ((uint32_t*) ((0x08000000 + 4096 * 1024) - 8 * 1024))
#define storedSettings ((menuView::Settings*) USER_FLASH_ADDRESS)

class menuView : public menuViewBase
{
public:
    menuView();
    virtual ~menuView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void options_solder_profile_wheelUpdateItem(custom_container_profile_select& item, int16_t itemIndex);

    struct Settings{
    	int openTemperature;
    };

private:

    constexpr static int maxProfileDuration = 600;
    int profileDurationS = 600;
    float profileTemperaturesS[maxProfileDuration];
    float measuredTemperaturesS[maxProfileDuration];

    int measureIndex = 0;

    SolderProfile selectedProfile;

    modal::Modal modalWrapper;
    UiErrorState errorState;

    bool isProfileSelected = false;
    bool startReflowEnabled = true;
    bool openLidEnabled = true;

    int shownCurrentState = -1; // core state shown in state_text, -1: none yet

    int SolderProfilesVersion = -1;
    std::vector<std::string> solderProfiles;
    //DynamicDrawableListItems<custom_container_profile_select>* solderProfileWheelItems = nullptr;

    DrawableListItems<custom_container_profile_select, 8> solderProfileWheelItemsFixed;

    touchgfx::Callback<menuView, touchgfx::DrawableListItemsInterface*, int16_t, int16_t> updateItemCallback;

protected:

    void close_lid_btn_onClick();
    void open_lid_btn_onClick();
    void start_reflow_btn_onClick();
    void stop_reflow_btn_onClick();

    void options_openlid_tem_button_eco_onClick();
    void options_openlid_tem_button_standard_onClick();
    void options_openlid_tem_button_fast_onClick();
    void options_button_profile_set_onClick();
    void options_sd_card_button_mount_onClick();
    void options_sd_card_button_eject_onClick();
    void options_temp_toggle_button_onClick();
    void modalWindowInformationClose_btn_onClick();
    void modalWindowDecisionYes_btn_onClick();
    void modalWindowDecisionNo_btn_onClick();
    void options_lid_service_button_onClick();
    void options_sd_card_button_export_onClick();



    void drawSetHeatProfileGraph();
    void adaptGraphToUnits();
    void adaptGraphTimeAxis();

    void TempUnitToggled();
    void adaptUiToUnits();

    void loadSettingsFromFlash();
    HAL_StatusTypeDef saveSettings();
    void restoreOpenTemperature();

    //void startErrorCallback();
    //void stopReflowDecisionCallback(bool answer);


    const uint32_t pcbTempUpdateIntervalMs = 1000;
    const uint32_t galdenTempUpdateIntervalMs = 1000;
    const uint32_t coolantTempUpdateIntervalMs = 1000;

    TemperatureUnit currentUnit = CELSIUS;
    float galdenTemperatureCelsius = 0.0f;
    float coolantTemperatureCelsius = 0.0f;

    void handleTickEvent();
    void updateCoolantTemperatureLabel();
    void updatePCBTemperatureLabel();
    void updateGaldenTemperatureLabel();
    void updateCurrentState();
    void updateLogging();
    void updateTemperatureGraph();
    void updateTemperatureAndState();
    void updateButtonStates();
    void updateEnabledBitmapsOnly(touchgfx::ImageButtonStyle<touchgfx::TouchButtonTrigger>* button, bool shouldBeEnabled,
    		const touchgfx::Bitmap& bmpReleased, const touchgfx::Bitmap& bmpPressed,
    		const touchgfx::Bitmap& bmpDisabled, bool* enabledState);
    void updateEnabled(touchgfx::ImageButtonStyle<touchgfx::TouchButtonTrigger> * button, bool shouldBeEnabled,
    		const touchgfx::Bitmap& bmpReleased, const touchgfx::Bitmap& bmpPressed,
    		const touchgfx::Bitmap& bmpDisabled);
    void updateVisibility(touchgfx::ImageButtonStyle<touchgfx::TouchButtonTrigger>* widget, bool desiredVisibility);
    void updateScrollWheel();
    void updateSolderProfiles();
    void updateSelftest();

    void updateItemCallbackHandler(touchgfx::DrawableListItemsInterface* items, int16_t containerIndex, int16_t itemIndex);





};

#endif // MENUVIEW_HPP
