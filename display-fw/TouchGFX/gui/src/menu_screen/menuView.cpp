#include <gui/menu_screen/menuView.hpp>
#include "BitmapDatabase.hpp"
#include <texts/TextKeysAndLanguages.hpp>
#include "touchgfx/widgets/Button.hpp"

#include "log.h"
#include "main.h"
#include "core-communication.hpp"
#include "sd.hpp"
#include "reflow_log.hpp"
#include "unitConverter.hpp"
//#include "UiErrorState.hpp"

#include <touchgfx/canvas_widget_renderer/CanvasWidgetRenderer.hpp>

menuView::Settings currentSettings;

menuView::menuView():
	updateItemCallback(this, &menuView::updateItemCallbackHandler)
{
	modal::ModalElements elements = {
		.modalWindowInformation = &modalWindowInformation,
		.modalWindowInformationClose = &modalWindowInformationClose,
		.modalWindowInformationBody = &modalWindowInformationBody,
		.modalWindowInformationTitle = &modalWindowInformationTitle,
		.modalWindowDecision = &modalWindowDecision,
		.modalWindowDecisionYes = &modalWindowDecisionYes,
		.modalWindowDecisionNo = &modalWindowDecisionNo,
		.modalWindowDecisionBody = &modalWindowDecisionBody,
		.modalWindowDecisionTitle = &modalWindowDecisionTitle,
	};
	modalWrapper = modal::Modal(elements);
	errorState.setModal(&modalWrapper);

	graph_target.clear();
	graph_target.invalidate();
	graph_actual.clear();
	graph_actual.invalidate();

	options_solder_profile_wheel.setNumberOfItems(0);

	loadSettingsFromFlash();
	restoreOpenTemperature();
	updateReflowLogSetting();
}

void menuView::restoreOpenTemperature(){
	switch(currentSettings.openTemperature){
	case 0:
		vpx_log("Open temperature mode: Eco");
		CoreCom_SetLidMode(VPX_ECO);

		options_openlid_tem_button_fast.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_FAST_ID), touchgfx::Bitmap(BITMAP_BUTTON_FAST_PUSHED_ID));
		options_openlid_tem_button_standard.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_ID), touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_PUSHED_ID));
		options_openlid_tem_button_eco.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_ECO_PUSHED_GREEN_ID), touchgfx::Bitmap(BITMAP_BUTTON_ECO_PUSHED_ID));

		options_openlid_tem_button_fast.invalidate();
		options_openlid_tem_button_standard.invalidate();
		options_openlid_tem_button_eco.invalidate();
		break;
	case 1:
		vpx_log("Open temperature mode: Standard");
		CoreCom_SetLidMode(VPX_STANDARD);

		options_openlid_tem_button_fast.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_FAST_ID), touchgfx::Bitmap(BITMAP_BUTTON_FAST_PUSHED_ID));
		options_openlid_tem_button_standard.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_PUSHED_GREEN_ID), touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_PUSHED_ID));
		options_openlid_tem_button_eco.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_ECO_ID), touchgfx::Bitmap(BITMAP_BUTTON_ECO_PUSHED_ID));

		options_openlid_tem_button_fast.invalidate();
		options_openlid_tem_button_standard.invalidate();
		options_openlid_tem_button_eco.invalidate();
		break;
	case 2:
		vpx_log("Open temperature mode: Fast");
		CoreCom_SetLidMode(VPX_FAST);

		options_openlid_tem_button_fast.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_FAST_PUSHED_GREEN_ID), touchgfx::Bitmap(BITMAP_BUTTON_FAST_PUSHED_ID));
		options_openlid_tem_button_standard.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_ID), touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_PUSHED_ID));
		options_openlid_tem_button_eco.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_ECO_ID), touchgfx::Bitmap(BITMAP_BUTTON_ECO_PUSHED_ID));

		options_openlid_tem_button_fast.invalidate();
		options_openlid_tem_button_standard.invalidate();
		options_openlid_tem_button_eco.invalidate();
		break;
	}
}

HAL_StatusTypeDef menuView::saveSettings(){
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef eraseInit = {0};
    uint32_t pageError = 0;


    // Page index within bank 2, the config page is the last page of the flash
    uint32_t targetAddress = (uint32_t)USER_FLASH_ADDRESS;
    uint32_t pageIndex = (targetAddress - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;

    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.Banks     = FLASH_BANK_2;
    eraseInit.Page      = pageIndex;
    eraseInit.NbPages   = 1;

    if (HAL_FLASHEx_Erase(&eraseInit, &pageError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return HAL_ERROR;
    }


    const uint8_t* src = reinterpret_cast<const uint8_t*>(&currentSettings);
    uint32_t flashAddr = (uint32_t)USER_FLASH_ADDRESS;
    size_t bytesRemaining = sizeof(menuView::Settings);

    while (bytesRemaining > 0)
    {
        uint64_t quadword[2] = {0};
        size_t copySize = (bytesRemaining >= 16) ? 16 : bytesRemaining;
        memcpy(quadword, src, copySize);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,
                              flashAddr,
                              (uint32_t)quadword) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }

        flashAddr      += 16;
        src            += 16;
        bytesRemaining -= copySize;
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

void menuView::loadSettingsFromFlash()
{
    memcpy(&currentSettings, storedSettings, sizeof(menuView::Settings));
}


void menuView::setupScreen()
{
    menuViewBase::setupScreen();
    graph_target.setTouchable(false);
    graph_actual.setTouchable(false);

    // Only here and on unit toggle: it rebuilds both graphs completely
    adaptUiToUnits();
}

void menuView::tearDownScreen()
{
    menuViewBase::tearDownScreen();
}

void menuView::updateTemperatureAndState(){
	this->updateCoolantTemperatureLabel();
	this->updatePCBTemperatureLabel();
	this->updateGaldenTemperatureLabel();
	this->updateCurrentState();
}

void menuView::handleTickEvent()
{
	this->updateTemperatureAndState();
	this->updateButtonStates();
	this->updateSolderProfiles();
	this->updateTemperatureGraph();
	this->updateLogging();

	errorState.ErrorStateWorker();
}

void menuView::TempUnitToggled(){
	//Attention: hacky solution as I cannot find a way to get the displayed state
	if(currentUnit == CELSIUS){
		vpx_log("Fahrenheit selected");
		currentUnit = FAHRENHEIT;
	}else{
		vpx_log("Celsius selected");
		currentUnit = CELSIUS;
	}

	adaptUiToUnits();
}

//ADD FUNCTION WHEN SELFTEST GPIO IS CONNECTED!
//void menuView::updateSelftest() {
//	Vpx_CoreStateTypeDef state = CoreCom_CoreState();
//
//	if(HAL_GPIO_ReadPin(SELFTEST_GPIO_Port, SELFTEST_Pin) == GPIO_PIN_RESET && !showingSelftestQuestion) {
//		showingSelftestQuestion = true;
//
//	    modalWrapper.sheduleDecisionModal(
//	    		touchgfx::TypedText(T_SELFTESTTITLE),
//				touchgfx::TypedText(T_SELFTESTQUESTIONBODY),
//				startReflowDecisionCallback);
//	}
//
//	if(state.selftest_active && !selftestActiveOpened) {
//		modalWrapper.showOverlayInfo(
//				touchgfx::TypedText(T_SELFTESTTITLE),
//				touchgfx::TypedText(T_SELFTESTRUNNINGBODY));
//
//		selftestActiveOpened = true;
//	} else if(!state.selftest_active && selftestActiveOpened) {
//		modalWrapper.hideOverlayInfo();
//
//		selftestActiveOpened = false;
//	}
//}



void menuView::adaptUiToUnits(){
	if(currentUnit == CELSIUS){
		touchgfx::Unicode::snprintf(cooling_temp_textBuffer2, COOLING_TEMP_TEXTBUFFER2_SIZE, "%s", "C");
		touchgfx::Unicode::snprintf(galden_temp_textBuffer2, GALDEN_TEMP_TEXTBUFFER2_SIZE, "%s", "C");
		touchgfx::Unicode::snprintf(pcb_temp_textBuffer2, PCB_TEMP_TEXTBUFFER2_SIZE, "%s", "C");
		touchgfx::Unicode::snprintf(label_degreeBuffer, LABEL_DEGREE_SIZE, "%s", "C");
	}else{
		touchgfx::Unicode::snprintf(cooling_temp_textBuffer2, COOLING_TEMP_TEXTBUFFER2_SIZE, "%s", "F" );
		touchgfx::Unicode::snprintf(galden_temp_textBuffer2, GALDEN_TEMP_TEXTBUFFER2_SIZE, "%s", "F");
		touchgfx::Unicode::snprintf(pcb_temp_textBuffer2, PCB_TEMP_TEXTBUFFER2_SIZE, "%s", "F");
		touchgfx::Unicode::snprintf(label_degreeBuffer, LABEL_DEGREE_SIZE, "%s", "F");
	}

	label_degree.invalidate();

	adaptGraphToUnits();
	drawSetHeatProfileGraph();
}


void startErrorCallback() {
	vpx_log("Failure message 'start conditions not met' acknowledged");
}

void openLidErrorCallback() {
	vpx_log("Failure message 'open lid conditions not met' acknowledged");
}

void stopReflowDecisionCallback(bool answer){
	if(answer) {
		CoreCom_StopReflow();
	}
}

void menuView::updateCoolantTemperatureLabel() {
	static uint32_t lastUpdateTime = 0;
	uint32_t currentTimeMS = HAL_GetTick();
	if(currentTimeMS - lastUpdateTime < coolantTempUpdateIntervalMs){
		return;
	}
	lastUpdateTime = currentTimeMS;

	DegreeCelcius coolant_temperature_celsius = CoreCom_CoolantTemperature() / 1000;
	float temp = celsiusToUnit(coolant_temperature_celsius, currentUnit);

	// Invalidate the old extent too, else a shorter text leaves stray pixels
	cooling_temp_text.invalidate();
	Unicode::snprintfFloat(cooling_temp_textBuffer1, COOLING_TEMP_TEXTBUFFER1_SIZE,"%.0f", temp);
	cooling_temp_text.resizeToCurrentText();
	cooling_temp_text.invalidate();
}

void menuView::updatePCBTemperatureLabel() {
	static uint32_t lastUpdateTime = 0;
	uint32_t currentTimeMS = HAL_GetTick();
	if(currentTimeMS - lastUpdateTime < pcbTempUpdateIntervalMs){
		return;
	}
	lastUpdateTime = currentTimeMS;

	DegreeCelcius pcb_temperature_celsius = CoreCom_PcbTemperature() / 1000;
	float temp = celsiusToUnit(pcb_temperature_celsius, currentUnit);

	// Invalidate the old extent too, else a shorter text leaves stray pixels
	pcb_temp_text.invalidate();
	Unicode::snprintfFloat(pcb_temp_textBuffer1, PCB_TEMP_TEXTBUFFER1_SIZE,"%.0f", temp);
	pcb_temp_text.resizeToCurrentText();
	pcb_temp_text.invalidate();
}

void menuView::updateGaldenTemperatureLabel() {
	static uint32_t lastUpdateTime = 0;
	uint32_t currentTimeMS = HAL_GetTick();
	if(currentTimeMS - lastUpdateTime < galdenTempUpdateIntervalMs){
		return;
	}
	lastUpdateTime = currentTimeMS;
	DegreeCelcius galden_temperature_celcius = CoreCom_LiquidTemperature() / 1000;
	float temp = celsiusToUnit(galden_temperature_celcius, currentUnit);

	// Invalidate the old extent too, else a shorter text leaves stray pixels
	galden_temp_text.invalidate();
	Unicode::snprintfFloat(galden_temp_textBuffer1, GALDEN_TEMP_TEXTBUFFER1_SIZE,"%.0f", temp);
	galden_temp_text.resizeToCurrentText();
	galden_temp_text.invalidate();

}

void menuView::updateCurrentState(){
	Vpx_CoreStateTypeDef coreState = CoreCom_CoreState();
	const int state = coreState.current_state;
	if(state == shownCurrentState){
		return;
	}
	shownCurrentState = state;

	const char* text;
	switch(state){
	case CS_Preheat:          text = "Preheat";     break;
	case CS_Soldering:        text = "Soldering";   break;
	case CS_AntiCondensation: text = "Quick cool";  break;
	case CS_Calibration:      text = "Calibrating"; break;
	default:                  text = "Idle";        break;
	}

	// Invalidate the old extent too, else a shorter text leaves stray pixels
	state_text.invalidate();
	Unicode::strncpy(state_textBuffer, text, STATE_TEXT_SIZE);
	state_text.resizeToCurrentText();
	state_text.invalidate();
}

//void menuView::adaptUiToUnits(){
//	if(currentUnit == CELSIUS){
//		touchgfx::Unicode::snprintf(cooling_temp_textBuffer2, COOLING_TEMP_TEXTBUFFER2_SIZE, "%s", "C");
//		touchgfx::Unicode::snprintf(galden_temp_textBuffer2, GALDEN_TEMP_TEXTBUFFER2_SIZE, "%s", "C");
//		touchgfx::Unicode::snprintf(pcb_temp_textBuffer2, PCB_TEMP_TEXTBUFFER2_SIZE, "%s", "C");
//		touchgfx::Unicode::snprintf(label_degreeBuffer, LABEL_DEGREE_SIZE, "%s", "C");
//	}else{
//		touchgfx::Unicode::snprintf(cooling_temp_textBuffer2, COOLING_TEMP_TEXTBUFFER2_SIZE, "%s", "F" );
//		touchgfx::Unicode::snprintf(galden_temp_textBuffer2, GALDEN_TEMP_TEXTBUFFER2_SIZE, "%s", "F");
//		touchgfx::Unicode::snprintf(pcb_temp_textBuffer2, PCB_TEMP_TEXTBUFFER2_SIZE, "%s", "F");
//		touchgfx::Unicode::snprintf(label_degreeBuffer, LABEL_DEGREE_SIZE, "%s", "F");
//	}
//	label_degree.invalidate();
////
////	Unicode::snprintfFloat(watersystem_tmpBuffer1, WATERSYSTEM_TMPBUFFER1_SIZE,"%.0f", celsiusToUnit(coolantTemperatureCelsius, currentUnit));
////	watersystem_tmp.resizeToCurrentText();
////	watersystem_tmp.invalidate();
////
////	Unicode::snprintfFloat(galden_tmpBuffer1, GALDEN_TMPBUFFER1_SIZE,"%.0f", celsiusToUnit(galdenTemperatureCelsius, currentUnit));
////	galden_tmp.resizeToCurrentText();
////	galden_tmp.invalidate();
////
////	adaptGraphToUnits();
////	drawSetHeatProfileGraph();
//}

void menuView::updateVisibility(touchgfx::ImageButtonStyle<touchgfx::TouchButtonTrigger>* widget, bool desiredVisibility) {
	if(widget->isVisible() != desiredVisibility) {
		widget->setVisible(desiredVisibility);
		widget->invalidate();
	}
}

void menuView::updateEnabled(touchgfx::ImageButtonStyle<touchgfx::TouchButtonTrigger> * button, bool shouldBeEnabled,
		const touchgfx::Bitmap& bmpReleased, const touchgfx::Bitmap& bmpPressed,
		const touchgfx::Bitmap& bmpDisabled) {

	if(button->isTouchable() == shouldBeEnabled) {
		return;
	}

	button->setTouchable(shouldBeEnabled);
	button->setBitmaps(shouldBeEnabled ?  bmpReleased : bmpDisabled,
						bmpPressed);
	button->invalidate();
}


void menuView::updateEnabledBitmapsOnly(touchgfx::ImageButtonStyle<touchgfx::TouchButtonTrigger>* button, bool shouldBeEnabled,
		const touchgfx::Bitmap& bmpReleased, const touchgfx::Bitmap& bmpPressed,
		const touchgfx::Bitmap& bmpDisabled, bool* enabledState) {

	if(*enabledState == shouldBeEnabled) {
		return;
	}

	button->setBitmaps(	shouldBeEnabled ?  bmpReleased : bmpDisabled,
						shouldBeEnabled ?  bmpPressed : bmpDisabled);
	button->invalidate();

	*enabledState = shouldBeEnabled;
}

void menuView::updateButtonStates() {
	Vpx_CoreStateTypeDef coreState = CoreCom_CoreState();

	updateVisibility(&start_reflow, !coreState.reflow_active);
	updateEnabledBitmapsOnly(&start_reflow, coreState.can_start_reflow && isProfileSelected,
			touchgfx::Bitmap(BITMAP_START_REFLOW_ID), touchgfx::Bitmap(BITMAP_START_REFLOW_PUSH_ID),
						touchgfx::Bitmap(BITMAP_START_REFLOW_DIABLED_ID),
						&startReflowEnabled);

	updateEnabled(&stop_reflow, coreState.can_stop_reflow,
			touchgfx::Bitmap(BITMAP_STOP_REFLOW_ID), touchgfx::Bitmap(BITMAP_STOP_REFLOW_PUSH_ID),
									touchgfx::Bitmap(BITMAP_STOP_REFLOW_DISABLED_ID));

	updateVisibility(&open_lid, coreState.lid_state != LID_STATE_OPEN && coreState.lid_state != LID_STATE_CLOSING);
	updateEnabledBitmapsOnly(&open_lid, coreState.can_open_lid && coreState.lid_state == LID_STATE_CLOSED,
			touchgfx::Bitmap(BITMAP_OPEN_LID_ID), touchgfx::Bitmap(BITMAP_OPEN_LID_PUSH_ID),
			touchgfx::Bitmap(BITMAP_OPEN_LID_DISABLED_ID),
			&openLidEnabled);

	updateEnabled(&close_lid, coreState.lid_state == LID_STATE_OPEN,
			touchgfx::Bitmap(BITMAP_CLOSE_LID_ID), touchgfx::Bitmap(BITMAP_CLOSE_LID_PUSH_ID),
			touchgfx::Bitmap(BITMAP_CLOSE_LID_DISABLED_ID));

	updateEnabled(&options_button_profile_set, !coreState.reflow_active,
			touchgfx::Bitmap(BITMAP_BUTTON_SET_ID), touchgfx::Bitmap(BITMAP_BUTTON_SET_PUSH_ID),
			touchgfx::Bitmap(BITMAP_BUTTON_SET_DISABLE_ID));

	updateVisibility(&options_sd_card_button_eject, SD_Mounted());
	updateVisibility(&options_sd_card_button_mount, !SD_Mounted());
}

void menuView::close_lid_btn_onClick()
{
	vpx_log("Closed lid pushed");
    CoreCom_CloseLid();
}

void menuView::open_lid_btn_onClick()
{
	Vpx_CoreStateTypeDef coreState = CoreCom_CoreState();
	vpx_log("Open lid pushed");

	if(!(coreState.can_open_lid && coreState.lid_state == LID_STATE_CLOSED)) {
			modalWrapper.sheduleInformationModal(
					touchgfx::TypedText(T_LIDCONDITIONERRORTITLE),
					touchgfx::TypedText(T_LIDCONDITIONERRORBODY),
					openLidErrorCallback);
		return;
	}

	CoreCom_OpenLid();
}

void menuView::start_reflow_btn_onClick(){
	Vpx_CoreStateTypeDef coreState = CoreCom_CoreState();
	vpx_log("Start reflow pushed");

	if(!(coreState.can_start_reflow && isProfileSelected)) {
		modalWrapper.sheduleInformationModal(
				touchgfx::TypedText(T_STARTCONDITIONERRORTITLE),
				touchgfx::TypedText(T_STARTCONDITIONERRORBODY),
				startErrorCallback);
		return;
	}

	measureIndex = 0;

	graph_actual.clear();
	graph_actual.invalidateContent();
	graph_actual.invalidate();

	graph_actual.addDataPoint(0);

	abortedProfileText.setVisible(false);
	abortedProfileBackground.setVisible(false);

	CoreCom_StartProfile(selectedProfile);
}

void menuView::stop_reflow_btn_onClick(){
	vpx_log("Stop reflow pushed");

    modalWrapper.sheduleDecisionModal(
    		touchgfx::TypedText(T_PROCESSCANCELTITLE),
			touchgfx::TypedText(T_PROCESSCANCELBODY),
			stopReflowDecisionCallback);
}

void menuView::options_openlid_tem_button_eco_onClick(){
	vpx_log("Open temperature mode: Eco");
	CoreCom_SetLidMode(VPX_ECO);

	options_openlid_tem_button_fast.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_FAST_ID), touchgfx::Bitmap(BITMAP_BUTTON_FAST_PUSHED_ID));
	options_openlid_tem_button_standard.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_ID), touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_PUSHED_ID));
	options_openlid_tem_button_eco.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_ECO_PUSHED_GREEN_ID), touchgfx::Bitmap(BITMAP_BUTTON_ECO_PUSHED_ID));

	options_openlid_tem_button_fast.invalidate();
	options_openlid_tem_button_standard.invalidate();
	options_openlid_tem_button_eco.invalidate();

	currentSettings.openTemperature = 0;
	this->saveSettings();
}

void menuView::options_openlid_tem_button_standard_onClick(){
	vpx_log("Open temperature mode: Standard");
	CoreCom_SetLidMode(VPX_STANDARD);

	options_openlid_tem_button_fast.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_FAST_ID), touchgfx::Bitmap(BITMAP_BUTTON_FAST_PUSHED_ID));
	options_openlid_tem_button_standard.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_PUSHED_GREEN_ID), touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_PUSHED_ID));
	options_openlid_tem_button_eco.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_ECO_ID), touchgfx::Bitmap(BITMAP_BUTTON_ECO_PUSHED_ID));

	options_openlid_tem_button_fast.invalidate();
	options_openlid_tem_button_standard.invalidate();
	options_openlid_tem_button_eco.invalidate();

	currentSettings.openTemperature = 1;
	this->saveSettings();
}

void menuView::options_openlid_tem_button_fast_onClick(){
	vpx_log("Open temperature mode: Fast");
	CoreCom_SetLidMode(VPX_FAST);

	options_openlid_tem_button_fast.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_FAST_PUSHED_GREEN_ID), touchgfx::Bitmap(BITMAP_BUTTON_FAST_PUSHED_ID));
	options_openlid_tem_button_standard.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_ID), touchgfx::Bitmap(BITMAP_BUTTON_STANDARD_PUSHED_ID));
	options_openlid_tem_button_eco.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_ECO_ID), touchgfx::Bitmap(BITMAP_BUTTON_ECO_PUSHED_ID));

	options_openlid_tem_button_fast.invalidate();
	options_openlid_tem_button_standard.invalidate();
	options_openlid_tem_button_eco.invalidate();

	currentSettings.openTemperature = 2;
	this->saveSettings();
    }

void menuView::options_sd_card_button_mount_onClick(){
     SD_Mount();
}

void menuView::options_sd_card_button_eject_onClick(){
	SD_SheduleUnmount();
}

void menuView::options_temp_toggle_button_onClick(){
	if(currentUnit == CELSIUS){
		vpx_log("Fahrenheit selected");
		currentUnit = FAHRENHEIT;
	}
	else{
		vpx_log("Celsius selected");
		currentUnit = CELSIUS;
	}

	this->adaptUiToUnits();
}

void menuView::updateSolderProfiles(){
	int currentSolderProfileVersion = GetFileLoadCounter();
	if(SolderProfilesVersion != currentSolderProfileVersion){
		solderProfiles = GetAvailableCSVFiles();
		SolderProfilesVersion = currentSolderProfileVersion;

		updateScrollWheel();
		isProfileSelected = false;
	}
}

void menuView::updateScrollWheel(){
	//if(solderProfileWheelItems != nullptr){
	//	delete solderProfileWheelItems;
	//}

	//int isolderProfileSize = solderProfiles.size();

	//solderProfileWheelItems = new DynamicDrawableListItems<custom_container_profile_select>(isolderProfileSize);

    volatile int16_t solderProfileSize = solderProfiles.size();
    options_solder_profile_wheel.setNumberOfItems(solderProfileSize);
	options_solder_profile_wheel.setDrawables(solderProfileWheelItemsFixed, updateItemCallback);
	options_solder_profile_wheel.animateToItem(0, 0);

	//options_solder_profile_wheel.initialize();
	for (int i = 0; i < solderProfileSize; i++)
	{
		(solderProfileWheelItemsFixed)[i].initialize();
	}
	options_solder_profile_wheel.invalidate();
}

void menuView::options_solder_profile_wheelUpdateItem(custom_container_profile_select& item, int16_t itemIndex)
{
	if(itemIndex>=solderProfiles.size()){
		std::string errorText = "no solder profile";
		item.setText(errorText);
	}
	else{
		std::string text = solderProfiles[itemIndex];
		text.erase(0,9);
		item.setText(text);
	}
	item.invalidate();
}

void menuView::updateItemCallbackHandler(touchgfx::DrawableListItemsInterface* items, int16_t containerIndex, int16_t itemIndex)
{
    if (items == &solderProfileWheelItemsFixed)
    {
        touchgfx::Drawable* d = items->getDrawable(containerIndex);
        custom_container_profile_select* cc = (custom_container_profile_select*)d;
        options_solder_profile_wheelUpdateItem(*cc, itemIndex);
    }
}

void menuView::options_button_profile_set_onClick(){

	abortedProfileText.setVisible(false);
	abortedProfileBackground.setVisible(false);

	int itemIndex = options_solder_profile_wheel.getSelectedItem();
		if (itemIndex >= solderProfiles.size()){
			updateScrollWheel();
			return;
		}
		isProfileSelected = false;

		measureIndex = 0;



		graph_actual.clear();
		graph_actual.invalidateContent();
		graph_actual.invalidate();

		graph_actual.addDataPoint(0);

		std::string profilePath = solderProfiles[itemIndex];
		selectedProfile = loadSolderCsv(profilePath);
		if(!selectedProfile.valid){
			if(selectedProfile.readError){
				SD_SheduleUnmount();
			} else {
				// the reason is in the log
				vpx_log("Profile %s is invalid", profilePath.c_str());
				modalWrapper.sheduleInformationModal(
						touchgfx::TypedText(T_INVALIDPROFILETITLE),
						touchgfx::TypedText(T_INVALIDPROFILEBODY));
			}
			return;
		}
		vpx_log("Profile %s selected", profilePath.c_str());
		isProfileSelected = true;

		int profileLength = selectedProfile.timeTemperatureData.size();

		for(int i = 0; i<maxProfileDuration;i++){
			profileTemperaturesS[i] = 0;
		}

		int maxTime = 0;
		int currentTime = 0;
		for(int i = 0; i < selectedProfile.timeTemperatureData.size() - 1; i++){
			float delta_temperature = selectedProfile.timeTemperatureData[i+1].temperature - selectedProfile.timeTemperatureData[i].temperature;
			float delta_time = selectedProfile.timeTemperatureData[i+1].time - selectedProfile.timeTemperatureData[i].time;
			float a = delta_temperature / delta_time;
			float b = selectedProfile.timeTemperatureData[i].temperature;

			for(int j = 0;currentTime < selectedProfile.timeTemperatureData[i+1].time;currentTime++, j++){
				profileTemperaturesS[currentTime] = a * j + b;
			}
		}
		profileTemperaturesS[currentTime] = selectedProfile.timeTemperatureData[selectedProfile.timeTemperatureData.size() - 1].temperature;
		profileDurationS = currentTime;

	    CoreCom_SendGalden(selectedProfile);		//Core communication to send Galden Boilingtemperature of .csv
		CoreCom_SendACT(selectedProfile);
		ReflowLog_SetProfile(profilePath.c_str(), profileTemperaturesS, profileDurationS);
	    drawSetHeatProfileGraph();
		adaptGraphTimeAxis();

		menu_container.setSelectedPage(1);

}

void menuView::drawSetHeatProfileGraph(){
	if(isProfileSelected){
		graph_target.clear();
		for (int i = 0; i < profileDurationS; i++)
		{
			graph_target.addDataPoint(celsiusToUnit(profileTemperaturesS[i], currentUnit));
		}
	}
}


void menuView::adaptGraphToUnits(){
	// Set the range for the x and y axis of the graph. That is
	// the max and min x/y value that can be displayed inside the
	// dimension of the graph.
	graph_actual.setGraphRangeY(celsiusToUnit(0, currentUnit), celsiusToUnit(250, currentUnit));
	graph_actual.invalidate();

	graph_target.setGraphRangeY(celsiusToUnit(0, currentUnit), celsiusToUnit(250, currentUnit));
	graph_target.invalidate();

	if(isProfileSelected){
		graph_actual.clear();
		for(int i = 0; i< measureIndex;i++){
			graph_actual.addDataPoint(celsiusToUnit(measuredTemperaturesS[i], currentUnit));
		}
	}
}

void menuView::adaptGraphTimeAxis(){
	graph_actual.setGraphRangeX(0, profileDurationS);
	graph_actual.invalidate();

	graph_target.setGraphRangeX(0, profileDurationS);
	graph_target.invalidate();
}

void menuView::modalWindowInformationClose_btn_onClick(){
	modalWrapper.closeModal(false);
}

void menuView::modalWindowDecisionYes_btn_onClick(){
	modalWrapper.closeModal(true);
}

void menuView::modalWindowDecisionNo_btn_onClick(){
	modalWrapper.closeModal(false);

}

void menuView::options_lid_service_button_onClick(){
	Vpx_CoreStateTypeDef coreState = CoreCom_CoreState();
	vpx_log("Service position pushed");

	// Same conditions as opening the lid. Not with the lid open: the lift doesn't move then.
	if(!(coreState.can_open_lid && coreState.lid_state == LID_STATE_CLOSED)) {
		modalWrapper.sheduleInformationModal(
				touchgfx::TypedText(T_LIDCONDITIONERRORTITLE),
				touchgfx::TypedText(T_LIDCONDITIONERRORBODY),
				openLidErrorCallback);
		return;
	}

	CoreCom_ServicePosition();
}

void menuView::options_sd_card_button_export_onClick(){
	// toggles "Reflow recording active"
	currentSettings.reflowLogEnabled = currentSettings.reflowLogEnabled == 1 ? 0 : 1;
	this->saveSettings();
	updateReflowLogSetting();
}

// The button shows the current state: yes = reflows are recorded
void menuView::updateReflowLogSetting(){
	const bool enabled = currentSettings.reflowLogEnabled == 1;
	vpx_log("Reflow recording active: %s", enabled ? "on" : "off");
	ReflowLog_SetEnabled(enabled);

	if(enabled){
		options_sd_card_button_export.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_YES_ID), touchgfx::Bitmap(BITMAP_BUTTON_YES_PUSHED_ID));
	} else {
		options_sd_card_button_export.setBitmaps(touchgfx::Bitmap(BITMAP_BUTTON_NO_ID), touchgfx::Bitmap(BITMAP_BUTTON_NO_PUSHED_ID));
	}
	// the images are 45 px high, the button 60 px
	options_sd_card_button_export.setBitmapXY(0, 7);
	options_sd_card_button_export.invalidate();
}

void menuView::updateLogging(){
	if(CoreCom_HasNewMonitor() && !CoreCom_CoreState().selftest_active){
		auto monitor = CoreCom_GetMonitorState();

		CoreCom_ReadMonitor();

		/*
		std::stringstream stream;
		stream.precision(2);

		stream << "HEATER1_TEMP:" << monitor.heater1_temp << ";";
		stream << "HEATER2_TEMP:" << monitor.heater2_temp << ";";
		stream << "GALDEN_TEMP:" << monitor.galden_temp << ";";
		stream << "PCB_TEMP:" << monitor.pcb_temp << ";";
		stream << "COOLANT_TEMP:" << monitor.coolant_temp << ";";

		vpx_log(stream.str().c_str());
		*/

		vpx_log("HEATER1_TEMP:%d", monitor.heater1_temp);
		vpx_log("HEATER2_TEMP:%d", monitor.heater2_temp);
		vpx_log("GALDEN_TEMP:%d", monitor.galden_temp);
		vpx_log("PCB_TEMP:%d", monitor.pcb_temp);
		vpx_log("COOLANT_TEMP:%d", monitor.coolant_temp);

		vpx_log("LID_COOLING_FAN:%d", monitor.lid_cooling_fan_on);
		vpx_log("COOLANT_PUMP:%d", monitor.coolant_pump_on);
		vpx_log("QC_FANS:%d", monitor.quick_cool_fan_on);
		vpx_log("RADIATOR_FANS:%d", monitor.radiator_fan_speed);


		vpx_log("LID_POS:%d", monitor.lid_position);
		vpx_log("LIFT_POS:%d", monitor.lift_position);

		vpx_log("HEATER:%d", monitor.heater_on);
		vpx_log("HEATER_OTP:%d", monitor.heater_otp_active);
		vpx_log("COOLANT_OTP:%d", monitor.coolant_otp_active);
		vpx_log("LID_LOCK:%d", monitor.lid_lock_active);
	}
}

void menuView::updateTemperatureGraph() {
	DegreeCelcius interpolate_to = CoreCom_PcbTemperature() / 1000;

	int time = CoreCom_CoreOperationTime();

	// negative times are invalid no update
	if(time < 0) {
		return;
	}

	int target_index = CoreCom_CoreOperationTime() / 1000;
	if (target_index < 0) {
		return;
	}

	if (target_index >= profileDurationS) {
		target_index = profileDurationS - 1;
	}

	// turnaround happend
	if (target_index < measureIndex - 1) {
		measureIndex = 0;
		graph_actual.clear();
		graph_actual.invalidate();
	}

	float interpolate_from = measureIndex <= 0 ? interpolate_to : measuredTemperaturesS[measureIndex - 1];

	int number_of_steps = (target_index - measureIndex) + 1;

	float step = number_of_steps <= 0 ? 0 : (interpolate_to - interpolate_from) / number_of_steps;

	for(; measureIndex <= target_index; measureIndex++) {
		interpolate_from += step;

		measuredTemperaturesS[measureIndex] = interpolate_from;
		graph_actual.addDataPoint(celsiusToUnit(interpolate_from, currentUnit));
	}
}





