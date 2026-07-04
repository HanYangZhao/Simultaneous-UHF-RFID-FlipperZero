#include "view_lock.h"
#include "view_kill.h"
/**
 * @brief      Callback when the user exits the lock screen.
 * @details    This function is called when the user exits the lock screen.
 * @param      context  The context - not used
 * @return     the view id of the next view.
*/
uint32_t uhf_reader_navigation_lock_callback(void* context) {
    UNUSED(context);
    return UHFReaderViewLock;
}

/**
 * @brief      Callback for the uhf worker for locking.
 * @details    This function is called when the uhf worker is started for locking.
 * @param      event  The UHFWorkerEvent - UHFReaderApp object.
 * @param      context  The context - UHFReaderApp object.
*/
void uhf_access_tag_worker_callback(UHFWorkerEvent event, void* context) {
    UHFReaderApp* App = (UHFReaderApp*)context;
    if(event == UHFWorkerEventSuccess) {
        dolphin_deed(DolphinDeedNfcReadSuccess);
        notification_message(App->Notifications, &uhf_sequence_blink_stop);
        notification_message(App->Notifications, &sequence_success);
        App->YRM100XWorker->AccessPwd = false;
        popup_reset(App->LockPopup);
        view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewLock);
    } else if(event == UHFWorkerEventAborted) {
        App->YRM100XWorker->AccessPwd = false;
        notification_message(App->Notifications, &uhf_sequence_blink_stop);
        notification_message(App->Notifications, &sequence_error);
        popup_reset(App->LockPopup);
        view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewLock);
    }
}
/**
 * @brief      Allocates the access password input view 
 * @details    Allocates the set access password input view 
 * @param      context The UHFReaderApp app - used to allocate app variables and views.
*/
void access_password_menu_alloc(UHFReaderApp* App) {
    App->SetApInput = byte_input_alloc();
    view_dispatcher_add_view(
        App->ViewDispatcher, UHFReaderViewSetAccessPwd, byte_input_get_view(App->SetApInput));
    App->SetPwdTempBuffer = (uint8_t*)malloc(4);
}
/**
 * @brief      Handles the set access password view.
 * @details    Used when the access password is set through the GUI
 * @param      context The UHFReaderApp app - used to allocate app variables and views.
*/
void uhf_reader_access_password_updated(void* context) {
    UHFReaderApp* App = (UHFReaderApp*)context;
    // Convert the 4-byte buffer to a hex string for display.
    char* tempBuffer = (char*)malloc(9);
    snprintf(tempBuffer, 9, "%s", convert_to_hex_string(App->SetPwdTempBuffer, 4));

    // Store the new AP in memory only — no write to tag.
    // To write the AP to the tag, use the Reserved bank in the Write view.
    App->YRM100XWorker->DefaultAP = bytes_to_uint32(App->SetPwdTempBuffer, 4);
    furi_string_set_str(App->DefaultLockAccessPwdStr, tempBuffer);
    variable_item_set_current_value_text(
        App->SettingLockApPwdItem, furi_string_get_cstr(App->DefaultLockAccessPwdStr));

    free(tempBuffer);
    view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewLock);
}

/**
 * @brief      Callback when the user exits the lock screen.
 * @details    This function is called when the user exits the lock screen.
 * @param      context  The context - not used
 * @return     the view id of the next view.
*/
uint32_t uhf_reader_navigation_lock_exit_callback(void* context) {
    UNUSED(context);
    return UHFReaderViewTagAction;
}

/**
 * @brief      Maps lock type to string 
 * @details    This function is used to change the lock type value for the lock popups.
 * @param      type LockType - The locktype 
 * @return     the string representing the lock type action
*/
const char* get_lock_bank_type_string(LockType type) {
    switch(type) {
    case Lock:
        return "Locking";
    case Unlock:
        return "Unlocking";
    case PermaUnlock:
        return "Perma Unlocking";
    case PermaLock:
        return "Perma Locking";
    default:
        return "Unknown";
    }
}

/**
 * @brief      Maps bank type to string 
 * @details    This function is used to change the bank type value for the lock popups.
 * @param      bank BankType - The bank type 
 * @return     the string representing the bank type
*/
const char* get_memory_bank_string(BankType bank) {
    switch(bank) {
    case ReservedBank:
        return "Reserved\nBank";
    case EPCBank:
        return "EPC\nBank";
    case TIDBank:
        return "TID\nBank";
    case UserBank:
        return "User\nBank";
    case KillPwd:
        return "Kill\nPassword";
    case AccessPwd:
        return "Access\nPassword";
    case FileZero:
        return "User\nBank";
    default:
        return "Unknown\nBank";
    }
}
/**
 * @brief      Handles the lock items clicked
 * @details    Handles hanldes the lock actions 
 * @param      context, index - context used for UHFReaderApp, index used for state check.
*/
void uhf_reader_lock_item_clicked(void* context, uint32_t index) {
    UHFReaderApp* App = (UHFReaderApp*)context;
    Popup* PopupLock = App->LockPopup;
    uint32_t returnResponse = 0;
    char* header = malloc(68 * sizeof(char));
    index++;

    //Check if the set AP menu is being selected
    if(index == 1) {
        // Header to display on the access password input screen.
        byte_input_set_header_text(App->SetApInput, App->SetAccessPasswordPlaceHolder);
        byte_input_set_result_callback(
            App->SetApInput,
            uhf_reader_access_password_updated,
            NULL,
            App,
            App->SetPwdTempBuffer,
            4);
        view_set_previous_callback(
            byte_input_get_view(App->SetApInput), uhf_reader_navigation_lock_callback);
        view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewSetAccessPwd);
    } else if(index == 4) {
        // Create a dynamic string for the popup header
        snprintf(
            header,
            68,
            "%s\n%s",
            get_lock_bank_type_string(App->DefaultLockType),
            get_memory_bank_string(App->DefaultLockBank));

        // Set the popup header with the dynamic values and switch to it
        popup_set_header(PopupLock, header, 68, 30, AlignLeft, AlignTop);
        popup_set_icon(PopupLock, 0, 3, &I_RFIDDolphinReceive_97x61);
        notification_message(App->Notifications, &uhf_sequence_blink_start_cyan);
        view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewLockPopup);

        while(true) {
            returnResponse = m100_lock_label_data(
                App->YRM100XWorker->module,
                App->DefaultLockBank,
                App->YRM100XWorker->DefaultAP,
                App->DefaultLockType);
            if(returnResponse == M100SuccessResponse) {
                notification_message(App->Notifications, &sequence_success);
                dolphin_deed(DolphinDeedNfcReadSuccess);
                break;
            } else if(returnResponse == M100APWrong) {
                notification_message(App->Notifications, &sequence_error);
                break;
            } else {
                // Any other error response — don't hang forever
                notification_message(App->Notifications, &sequence_error);
                break;
            }
        }
        notification_message(App->Notifications, &uhf_sequence_blink_stop);
        popup_reset(App->LockPopup);
        view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewLock);
        free(header);
    }
}

/**
 * @brief      Handles the UHF Bank Selection
 * @details    This function handles switching between different memory banks for locking the UHF RFID Tag
 * @param      item  VariableItem - the current selection for the bank.
*/
void uhf_reader_setting_lock_bank_change(VariableItem* Item) {
    UHFReaderApp* App = variable_item_get_context(Item);
    uint8_t Index = variable_item_get_current_value_index(Item);

    if(Index == 1) {
        App->DefaultLockBank = AccessPwd;
    } else if(Index == 2) {
        App->DefaultLockBank = EPCBank;
    } else if(Index == 3) {
        App->DefaultLockBank = TIDBank;
    } else if(Index == 4) {
        App->DefaultLockBank = FileZero;
    } else {
        App->DefaultLockBank = KillPwd;
    }
    variable_item_set_current_value_text(Item, App->SettingLockBankNames[Index]);
}

/**
 * @brief      Handles the UHF Lock Action Selection
 * @details    This function handles switching between different supported lock actions for the selected UHF RFID Memory Bank.
 * @param      item  VariableItem - the current selection for the lock action
*/
void uhf_reader_setting_lock_action_change(VariableItem* Item) {
    UHFReaderApp* App = variable_item_get_context(Item);
    uint8_t Index = variable_item_get_current_value_index(Item);
    if(Index == 1) {
        App->DefaultLockType = PermaUnlock;
    } else if(Index == 2) {
        App->DefaultLockType = Lock;
    } else if(Index == 3) {
        App->DefaultLockType = PermaLock;
    } else {
        App->DefaultLockType = Unlock;
    }
    variable_item_set_current_value_text(Item, App->SettingLockActionNames[Index]);
}
/**
 * @brief      Navigation callback returning to the current-AP input screen.
*/
static uint32_t uhf_reader_navigation_new_ap_callback(void* context) {
    UNUSED(context);
    return UHFReaderViewCurrentApInput;
}

/**
 * @brief      Navigation callback returning to the current-KP input screen.
*/
static uint32_t uhf_reader_navigation_new_kp_callback(void* context) {
    UNUSED(context);
    return UHFReaderViewCurrentKpInput;
}

/**
 * @brief      Called after the user confirms the current access password.
 *             Stores the value and shows the "new access password" byte input.
*/
void uhf_reader_current_ap_updated(void* context) {
    UHFReaderApp* App = (UHFReaderApp*)context;
    byte_input_set_header_text(App->NewApInput, "New Access Pwd");
    byte_input_set_result_callback(
        App->NewApInput, uhf_reader_new_ap_updated, NULL, App, App->NewApBuffer, 4);
    view_set_previous_callback(
        byte_input_get_view(App->NewApInput), uhf_reader_navigation_new_ap_callback);
    view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewNewApInput);
}

/**
 * @brief      Called after the user confirms the new access password.
 *             Performs a blocking write to the tag's Reserved bank (SA=2, DL=2).
*/
void uhf_reader_new_ap_updated(void* context) {
    UHFReaderApp* App = (UHFReaderApp*)context;
    if(!App->ReaderConnected) {
        notification_message(App->Notifications, &sequence_error);
        view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewTagAction);
        return;
    }
    Popup* popup = App->LockPopup;
    popup_reset(popup);
    popup_set_header(popup, "Updating\nAccess\nPwd...", 68, 30, AlignLeft, AlignTop);
    popup_set_icon(popup, 0, 3, &I_RFIDDolphinReceive_97x61);
    notification_message(App->Notifications, &uhf_sequence_blink_start_cyan);
    view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewLockPopup);
    uhf_reader_fetch_selected_tag(App);
    uint32_t current_ap = bytes_to_uint32(App->CurrentApBuffer, 4);
    M100ResponseType ret =
        m100_write_access_pwd(App->YRM100XWorker->module, current_ap, App->NewApBuffer);
    notification_message(App->Notifications, &uhf_sequence_blink_stop);
    popup_reset(popup);
    if(ret == M100SuccessResponse) {
        App->YRM100XWorker->DefaultAP = bytes_to_uint32(App->NewApBuffer, 4);
        notification_message(App->Notifications, &sequence_success);
    } else {
        notification_message(App->Notifications, &sequence_error);
    }
    view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewTagAction);
}

/**
 * @brief      Called after the user confirms the current kill password.
 *             Stores the value and shows the "new kill password" byte input.
*/
void uhf_reader_current_kp_updated(void* context) {
    UHFReaderApp* App = (UHFReaderApp*)context;
    byte_input_set_header_text(App->NewKpInput, "New Kill Pwd");
    byte_input_set_result_callback(
        App->NewKpInput, uhf_reader_new_kp_updated, NULL, App, App->NewKpBuffer, 4);
    view_set_previous_callback(
        byte_input_get_view(App->NewKpInput), uhf_reader_navigation_new_kp_callback);
    view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewNewKpInput);
}

/**
 * @brief      Called after the user confirms the new kill password.
 *             Performs a blocking write to the tag's Reserved bank (SA=0, DL=2).
*/
void uhf_reader_new_kp_updated(void* context) {
    UHFReaderApp* App = (UHFReaderApp*)context;
    if(!App->ReaderConnected) {
        notification_message(App->Notifications, &sequence_error);
        view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewTagAction);
        return;
    }
    Popup* popup = App->LockPopup;
    popup_reset(popup);
    popup_set_header(popup, "Updating\nKill\nPwd...", 68, 30, AlignLeft, AlignTop);
    popup_set_icon(popup, 0, 3, &I_RFIDDolphinReceive_97x61);
    notification_message(App->Notifications, &uhf_sequence_blink_start_cyan);
    view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewLockPopup);
    uhf_reader_fetch_selected_tag(App);
    uint32_t current_ap = bytes_to_uint32(App->CurrentKpBuffer, 4);
    M100ResponseType ret =
        m100_write_kill_pwd_only(App->YRM100XWorker->module, current_ap, App->NewKpBuffer);
    notification_message(App->Notifications, &uhf_sequence_blink_stop);
    popup_reset(popup);
    if(ret == M100SuccessResponse) {
        App->YRM100XWorker->DefaultKP = bytes_to_uint32(App->NewKpBuffer, 4);
        notification_message(App->Notifications, &sequence_success);
    } else {
        notification_message(App->Notifications, &sequence_error);
    }
    view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewTagAction);
}

/**
 * @brief      Allocates the four ByteInput views used by Update AP and Update KP.
*/
void update_pwd_views_alloc(UHFReaderApp* App) {
    App->CurrentApInput = byte_input_alloc();
    view_dispatcher_add_view(
        App->ViewDispatcher,
        UHFReaderViewCurrentApInput,
        byte_input_get_view(App->CurrentApInput));
    App->CurrentApBuffer = (uint8_t*)malloc(4);

    App->NewApInput = byte_input_alloc();
    view_dispatcher_add_view(
        App->ViewDispatcher, UHFReaderViewNewApInput, byte_input_get_view(App->NewApInput));
    App->NewApBuffer = (uint8_t*)malloc(4);

    App->CurrentKpInput = byte_input_alloc();
    view_dispatcher_add_view(
        App->ViewDispatcher,
        UHFReaderViewCurrentKpInput,
        byte_input_get_view(App->CurrentKpInput));
    App->CurrentKpBuffer = (uint8_t*)malloc(4);

    App->NewKpInput = byte_input_alloc();
    view_dispatcher_add_view(
        App->ViewDispatcher, UHFReaderViewNewKpInput, byte_input_get_view(App->NewKpInput));
    App->NewKpBuffer = (uint8_t*)malloc(4);
}

/**
 * @brief      Frees the four ByteInput views allocated by update_pwd_views_alloc.
*/
void update_pwd_views_free(UHFReaderApp* App) {
    view_dispatcher_remove_view(App->ViewDispatcher, UHFReaderViewCurrentApInput);
    byte_input_free(App->CurrentApInput);
    free(App->CurrentApBuffer);

    view_dispatcher_remove_view(App->ViewDispatcher, UHFReaderViewNewApInput);
    byte_input_free(App->NewApInput);
    free(App->NewApBuffer);

    view_dispatcher_remove_view(App->ViewDispatcher, UHFReaderViewCurrentKpInput);
    byte_input_free(App->CurrentKpInput);
    free(App->CurrentKpBuffer);

    view_dispatcher_remove_view(App->ViewDispatcher, UHFReaderViewNewKpInput);
    byte_input_free(App->NewKpInput);
    free(App->NewKpBuffer);
}

/**
 * @brief      Allocates the lock view
 * @details    This function allocates all variables for the lock view.
 * @param      app  The UHFReaderApp object.
*/
void view_lock_alloc(UHFReaderApp* App) {
    //Setting variables
    App->SettingApLabel = "AP";
    App->SetAccessPasswordPlaceHolder = strdup("Enter Access Password!");
    App->SettingApDefaultPassword = strdup("00000000");

    //Options for different banks to lock following the Gen2 Protocol Standard https://www.gs1.org/sites/default/files/docs/epc/Gen2_Protocol_Standard.pdf
    App->SettingLockBankConfigLabel = "Memory Bank";
    App->SettingLockBankValues[0] = 1;
    App->SettingLockBankValues[1] = 2;
    App->SettingLockBankValues[2] = 3;
    App->SettingLockBankValues[3] = 4;
    App->SettingLockBankValues[4] = 5;
    App->SettingLockBankNames[0] = "Kill";
    App->SettingLockBankNames[1] = "AP";
    App->SettingLockBankNames[2] = "EPC";
    App->SettingLockBankNames[3] = "TID";
    App->SettingLockBankNames[4] = "User";
    App->DefaultLockBank = KillPwd;

    //Options for the lock mode
    App->SettingLockActionConfigLabel = "Lock Mode";
    App->SettingLockActionValues[0] = 1;
    App->SettingLockActionValues[1] = 2;
    App->SettingLockActionValues[2] = 3;
    App->SettingLockActionValues[3] = 4;
    App->SettingLockActionNames[0] = "Unlock";
    App->SettingLockActionNames[1] = "Perm-U";
    App->SettingLockActionNames[2] = "Lock";
    App->SettingLockActionNames[3] = "Perm-L";
    App->DefaultLockType = Unlock;
    //The Button for executing the desired lock command and storing the output
    App->SettingLockExecuteConfigLabel = "Execute";
    App->SettingLockExecuteResult = strdup("Press Me!");

    //Allocating the set access password menu
    access_password_menu_alloc(App);

    //Allocating the Update AP/KP byte input views
    update_pwd_views_alloc(App);

    //Allocating the popup shown when locking the tags
    App->LockPopup = popup_alloc();
    view_dispatcher_add_view(
        App->ViewDispatcher, UHFReaderViewLockPopup, popup_get_view(App->LockPopup));

    //Creating the variable item list for the lock menu options
    App->VariableItemListLock = variable_item_list_alloc();
    variable_item_list_reset(App->VariableItemListLock);

    //The set access password menu
    App->DefaultLockAccessPwdStr = furi_string_alloc_set(App->SettingApDefaultPassword);
    App->SettingLockApPwdItem =
        variable_item_list_add(App->VariableItemListLock, App->SettingApLabel, 1, NULL, NULL);
    variable_item_set_current_value_text(
        App->SettingLockApPwdItem, furi_string_get_cstr(App->DefaultLockAccessPwdStr));
    variable_item_list_set_enter_callback(
        App->VariableItemListLock, uhf_reader_lock_item_clicked, App);

    VariableItem* Item = variable_item_list_add(
        App->VariableItemListLock,
        App->SettingLockBankConfigLabel,
        COUNT_OF(App->SettingLockBankValues),
        uhf_reader_setting_lock_bank_change,
        App);

    //Creating the default index for setting one which is the connection status
    App->SettingLockBankIndex = 0;
    variable_item_set_current_value_index(Item, App->SettingLockBankIndex);
    variable_item_set_current_value_text(
        Item, App->SettingLockBankNames[App->SettingLockBankIndex]);

    VariableItem* ItemLock = variable_item_list_add(
        App->VariableItemListLock,
        App->SettingLockActionConfigLabel,
        COUNT_OF(App->SettingLockActionValues),
        uhf_reader_setting_lock_action_change,
        App);

    //Creating the default index for setting one which is the connection status
    App->SettingLockActionIndex = 0;
    variable_item_set_current_value_index(ItemLock, App->SettingLockActionIndex);
    variable_item_set_current_value_text(
        ItemLock, App->SettingLockActionNames[App->SettingLockActionIndex]);

    //The execute button and result
    App->DefaultLockResultStr = furi_string_alloc_set(App->SettingLockExecuteResult);
    App->SettingLockResultItem = variable_item_list_add(
        App->VariableItemListLock, App->SettingLockExecuteConfigLabel, 1, NULL, NULL);
    variable_item_set_current_value_text(
        App->SettingLockResultItem, furi_string_get_cstr(App->DefaultLockResultStr));
    variable_item_list_set_enter_callback(
        App->VariableItemListLock, uhf_reader_lock_item_clicked, App);
    view_set_previous_callback(
        variable_item_list_get_view(App->VariableItemListLock),
        uhf_reader_navigation_lock_exit_callback);
    view_dispatcher_add_view(
        App->ViewDispatcher,
        UHFReaderViewLock,
        variable_item_list_get_view(App->VariableItemListLock));
}

/**
 * @brief      Frees the tag action view.
 * @details    This function frees all variables for the tag actions view.
 * @param      context  The context - UHFReaderApp object.
*/
void view_lock_free(UHFReaderApp* App) {
    view_dispatcher_remove_view(App->ViewDispatcher, UHFReaderViewLockPopup);
    popup_free(App->LockPopup);
    view_dispatcher_remove_view(App->ViewDispatcher, UHFReaderViewSetAccessPwd);
    byte_input_free(App->SetApInput);
    free(App->SetPwdTempBuffer);
    update_pwd_views_free(App);
    view_dispatcher_remove_view(App->ViewDispatcher, UHFReaderViewLock);
    variable_item_list_free(App->VariableItemListLock);
}
