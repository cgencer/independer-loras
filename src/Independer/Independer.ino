#include "heltec.h"
#include "device/mb-utils.h"
#include "device/mb-gui.h"
#include "Cipher.h"
#include "device/mb-crypt.h"
#include "device/mb-lora.h"

// TODO: Gateway Empfangen Layout fix
// TODO: Gateway und Actor Fehler beim Empfang irgendwo anzeigen

// TODO: Sendekontigent mit echter Zeit

// TODO: Feature Python WebServer
// TODO: Pakete doppelt senden um empfang besser zu macen
// TODO: Bug Senden fällt bei Actoren im Background aus (vielleicht mal bei Verwendung von sendLaster bei Near me ?)

// TODO: Warum Faktor 2 und 8 Multi Task Decoding

// TODO: Speicherüberlauf Schutz bei Actor App Umgebung Scan und Gateway Store Message und Background Actor Empfang
// TODO: Verbesser Start/Stop Actor Multi in Briefe abrufen
// TODO: UI Verbessern Leerzeichen bei Long Text ausgabe und Text Eingabe

// TODO: Refactoring c_id und Gateway id
// TODO: Refactoring disableShortcuts bei gui_selection wieder ausbauen, hat nicht funktioniert

/*
 * ####################################
 *  Config Section
 * ####################################
 */
// Product Config
String c_product_version = "v.0.1.5";
boolean c_dev_mode = false;
boolean c_actor_mode = true;

/*
 * ####################################
 *  State Section
 * ####################################
 */
String state_my_id = "0x01";      // Saved in db
String state_gateway_id = "0g01"; // Saved in db
String state_wifi_ssid = "";      // Saved in db
String state_wifi_pw = "";        // Saved in db

String state_wifi_hostname = "independer-" + String(rand());

int state_lora_gain = 20; // Supported values are between 2 and 17 for PA_OUTPUT_PA_BOOST_PIN, 0 and 14 for PA_OUTPUT_RFO_PIN - Saved in db

int state_oled_brightness = 255; //saved in db

/*
 * ####################################
 *  Setup Section
 * ####################################
 */

// Database
#include "device/mb-database.h"

// OTA
#include "device/mb-ota.h"

// Common
#include "application/application-independer.h"
#include "workflow/workflow-independer.h"

// Actor
#include "workflow/workflow-multi-actor.h"
#include "application/application-actor.h"
#include "workflow/workflow-actor.h"

// Gateway
#include "application/application-gateway.h"
#include "workflow/workflow-gateway.h"

void setup()
{

  boolean isFirstBoot = workflow_independer_init(c_actor_mode, c_product_version, c_dev_mode);

  if (!c_dev_mode)
  {
    if (isFirstBoot or !c_actor_mode)
    { // Show every boot on gateway
      gui_logo_static(c_product_version, state_my_id, state_gateway_id, c_actor_mode);
      delay(C_GUI_DELAY_STATIC);
    }
  }
}

/*
 * ####################################
 *  Loop Section
 * ####################################
 */
void loop()
{

  if (c_actor_mode)
  {
    multi_actor_start();
    workflow_actor_main_menu();
  }
  else
  {
    workflow_gateway_main();
  }
}
