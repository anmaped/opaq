/*
 *  Opaq is an Open AQuarium Controller firmware. It has been developed for
 *  supporting several aquarium devices such as ligh dimmers, power management
 *  outlets, water sensors, and peristaltic pumps. The main purpose is to
 *  control fresh and salt water aquariums.
 *
 *    Copyright (c) 2015 Andre Pedro. All rights reserved.
 *
 *  This file is part of opaq firmware for aquarium controllers.
 *
 *  opaq firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  opaq firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with opaq firmware.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
 
#include "AcHtml.h"

#include "OPAQ_controller.h"

AcHtml::AcHtml()
{
  
}

const __FlashStringHelper*  AcHtml::get_begin()
{    
  return F(" <!DOCTYPE html><html lang=\"en\"> ");
}

const __FlashStringHelper*  AcHtml::get_end()
{
  return F(" </html> ");
}

const __FlashStringHelper* AcHtml::get_header_light()
{
  // return string with menu
  return F(" \
    <head> \
    <title>Opaq C1</title> \
    <meta charset=\"utf-8\" /> \
    <link rel=\"stylesheet\" href=\"style.css\"> \
    <script src=\"Chart.min.js\"></script> \
    </head> ");
}

const __FlashStringHelper* AcHtml::get_header()
{
  // return string with menu
  return F(" \
    <head> \
    <title>Opaq C1</title> \
    <meta charset=\"utf-8\" /> \
    <link rel=\"stylesheet\" href=\"style.css\"> \
    </head> ");
}

const __FlashStringHelper* AcHtml::get_menu()
{
  static const char ss[] PROGMEM = (R"=====(
  <script>
  function sendGet(msg) {
      var xmlHttp = new XMLHttpRequest();
      
      xmlHttp.open( "GET", msg, false );
      xmlHttp.send( null );
  };
  function store() {
      sendGet("\global?storesettings=" + 1);
  };
  function selectLDevice(elem) {
    sendGet( "\light?sdevice=" +  elem[elem.selectedIndex].value  + "&setcurrent=true" );
    location.reload(true);
  };
  </script>
  <img height="100px" src="logo.png"/>
  <nav class="dropdownmenu">
  <ul>
    <li><a href="..">Opaq Status</a></li><!--
    --><li><a href="#">Settings</a>
    <ul id="submenu">
        <li><a href="light">Light-CO2</a></li>
        <!--<li><a href="co2">CO2-PH</a></li>-->
        <li><a href="advset">Advanced Settings</a></li>
      </ul>
    </li><!--
    --><li><a href="https://github.com/anmaped/opaq">Opaq on GitHub</a></li>
  </ul>
</nav>
  )=====");
  
  return FPSTR(&ss[0]);
}

const __FlashStringHelper* AcHtml::get_body_begin()
{
  return F("<body>");
}

const __FlashStringHelper* AcHtml::get_body_end()
{
  return F("</body>");
}

void AcHtml::get_light_script(AcStorage * const lstorage, String * const str)
{
  *str += F("<canvas class=\"lightsignals\" id=\"light\"></canvas><font size=\"4\">Current setting: </font><select onchange=\"selectLDevice(this);\" id=\"selDev\">");
  
  gen_listbox_lightdevices(lstorage, str);

  static const char script_chart[] PROGMEM = R"=====(
  </select> <button onclick="store()">Set Permanent</button>
  <script>
    var buyerData = {
    labels : ["0am","","","", "4am","","","","8am","","","","12am","","","","4pm","","","","8pm","","","","12pm"],
    labelsspace : 1,
  )=====";

  static const char script_chart2[] PROGMEM = R"=====(
    ]
  }		
  var buyers = document.getElementById('light').getContext('2d');
  new Chart(buyers).Line(buyerData);
  </script>
  )=====";
  
  *str += FPSTR(&script_chart[0]);
  *str += F("device : ");
  *str += String( idx2id( lstorage->getCurrentSelLDevice() ) );
  *str += F(",");
  *str += F("datasets : [");

  // colors
  char colorL[4][7] = { "354F64", "AAB0A2", "F9FDA9", "A57E39" };
  char colorP[4][7] = { "283743", "9CAB89", "4D5013", "845500" };
  
  if (lstorage->getNLDevice() > 0)
  {
    for(int s=0; s < N_SIGNALS; s++)
    {
      
      int lend = lstorage->getLDeviceSignal( s, S_LEN_EACH-1 );
      if (lend != 0)
      {
        *str += F("{");
  
        *str += F("fillColor : \"rgba(172,194,132,0)\",");
        *str += F("strokeColor : \"#");
        *str += String( colorL[ s % 4 ] );
        *str += F("\",");
        *str += F("pointColor : \"#fff\",");
        *str += F("pointStrokeColor : \"#");
        *str += String( colorP[ s % 4 ] );
        *str += F("\",");

        *str += F("holdY: false,");
        *str += F("data : [");
        
        for (int i=0; i < lend ; i++)
        {
          *str += F("[");
          *str += String( lstorage->getLDeviceSignal( s, i*2 ) );
          *str += F(",");
          *str += String( lstorage->getLDeviceSignal( s, i*2+1 ) );
          *str += F(",[");
          *str += String(s+1);
          *str += F(",");
          *str += String(i+1);
          *str += F("]],");
        }
        
        *str += F("]},");
      }
    }
  }


  // get all power devices states that were assigned to the current light device
  char colorarrayLines[4][7] = { "FFAAAA", "8685BA", "F9FDA9", "A57E39" };
  char colorarrayPoints[4][7] = { "550000", "11103B", "4D5013", "845500" };
  // draw step functions
  const uint8_t value = 245;
  uint8_t limit = 0;
  
  for ( int limit=0; limit < 16; limit += 2 )
    for ( int idx=0; idx < lstorage->getNumberOfPowerDevices() ; idx++ )
    {
      *str += F("{");
    
      *str += F("fillColor : \"rgba(172,194,132,0)\",");
      *str += F("strokeColor : \"#");
      *str += String( colorarrayLines[ idx % 4 ] );
      *str += F("\",");
      *str += F("pointColor : \"#fff\",");
      *str += F("pointStrokeColor : \"#");
      *str += String( colorarrayPoints[ idx % 4 ] );
      *str += F("\",");

      *str += F("holdY: true,");
      *str += F("data : [");
      
      for ( int step=limit; step < lstorage->getPDeviceStepSize(idx) && step < limit+2; step++ )
      {
        *str += F("[");
        *str += String( value-(idx*7) );
        *str += F(",");
        *str += String( lstorage->getPDeviceStep( idx, step ) );
        
        *str += F(",[");
        *str += String( idx + 1 );
        *str += F(",");
        *str += String( step + 1 );
        *str += F("]],");

      }

      *str += F("]},");
      
    }

  
  *str += FPSTR(&script_chart2[0]);
}

const __FlashStringHelper* AcHtml::get_light_settings_mbegin()
{
  static const char m[] PROGMEM = R"=====(
  <br><br>
    <div class="settings">
    <label><font size="4">Current Clock:</font>
  )=====";
  
  return FPSTR(&m[0]);
}

void AcHtml::get_light_settings_mclock(String *str, const RtcDateTime clock)
{
  char tmp[10] = "";
  sprintf(tmp," %02d:%02d.%02d", clock.Hour(), clock.Minute(), clock.Second());
  *str += tmp;
}

const __FlashStringHelper* AcHtml::get_light_settings_mend()
{
  static const char m2[] PROGMEM = R"=====( </label> <button style="height:30px; width:30px"><img src="clock.png" /></button> <text><font size="4"> | Clock Shift:</font></text> <select>
      <option value="none">none</option>
      <option value="onehour">+1hour</option>
      <option value="twohour">+2hour</option>
      <option value="threehour">+3hour</option>
      <option value="fourhour">+4hour</option>
    </select> <font size="4">|</font> <img src="tempicon.png"/>
    
  )=====";
  
  return FPSTR(&m2[0]);
}


void AcHtml::get_advset_light1(String *str)
{
  *str += F("<script type=\"text/javascript\">");
  *str += F("function toggle(id) \{");
  *str += F("var e = document.getElementById(id);");
  *str += F("if (e.style.display == \'\') e.style.display = \'none\'; else  e.style.display = \'\';}");
  *str += F("</script>");
  *str += F("<div class=\"settings\" id=\"settings\"><h2><a href=\"#\" onclick=\"toggle('wrapper')\">Light Controller</a></h2><div id=\"wrapper\" class=\"wrapper\" style=\"display:none\">Devices:<table style=\"width:100%;border: 1px solid black;border-collapse: collapse;\"><tr><td>Id</td><td>Type</td><td>CodeID</td><td>Linear</td><td></td><td></td></tr>");
  /*
function toggle(id) {
  var e = document.getElementById(id);
 
  if (e.style.display == '')
    e.style.display = 'none';
  else
    e.style.display = '';
} 
</script>
<div class="settings" id="settings">
<h2><a href="#" onclick="toggle('wrapper')">Light Controller</a></h2>
<div id="wrapper" class="open" style="display:none">
Devices:<table style="width:100%;border: 1px solid black;border-collapse: collapse;">
<tr>
  <td>Id</td><td>Type</td><td>CodeID</td><td>Linear</td><td></td><td></td>
</tr>
  )=====";
  
  return lset;*/
}

void AcHtml::get_advset_light_device(unsigned int n_ldevices, AcStorage::deviceLightDescriptor *device, String* a)
{ 
  if (n_ldevices == 0)
    return;
  
  // create row
  
  for(int i=0; i < n_ldevices; i++){
    *a += F("<tr>");
    *a += F("<td>");
    *a += String(device[i].id);
    *a += F("</td>");
    
    *a += F("<td>");
    *a += F("<select onchange=\"setDeviceType(this,");
    *a += String(i+1);
    *a += F(");\"><option value=\"");
    *a += String(ZETLIGHT_LANCIA_2CH);
    *a += F("\" ");
    if(device[i].type == ZETLIGHT_LANCIA_2CH)
    {
      *a += F("selected");
    }
    *a += F(">Zetlight Lancia</option><option value=\"");
    *a += String(OPENAQV1);
    *a += F("\" ");
    if(device[i].type == OPENAQV1)
    {
      *a += F("selected");
    }
    *a += F(">OpenAQ v1</option></select>");
    
    *a += F("</td>");
    
    *a += F("<td>");
    
    char tmp[5];
    for (int j=0; j < LIGHT_CODE_ID_LENGTH; j++)
    {
      sprintf(tmp, "%02x ", device[i].codeid[j]);
      *a += String(tmp);
    }
    
    *a += F("</td>");
    
    *a += F("<td>");
    *a += F("<input type=\"checkbox\" onclick=\"setDeviceOperation(this);\"");
    if(device[i].linear)
    {
      *a += F(" checked/>");
    }
    else
    {
      *a += F("/>");
    }
    *a += F("</td>");
    
    *a += F("<td><button onclick=\"bindLDevice(");
    *a += String(i);
    *a += F(");\">Bind</button><button onclick=\"unbindLDevice(");
    *a += String(i);
    *a += F(");\">Unbind</button></td><td>");
    if ( i == n_ldevices-1)
    {
      *a += F("<button onclick=\"removeDevice();\">Remove</button>");
    }
    *a += F("</td></tr>");
  }
  
}


void AcHtml::get_advset_light2(AcStorage * const lstorage, String * const str)
{
  *str += F("</table><input id=\"adevice\" type=\"button\" value=\"Add Device\" onclick=\"addDevice();\" />");

  if(lstorage->getNLDevice())
    *str += F("<br><br><h3>Light Settings (Points of lines or splines)</h3>Device Selected: <select onchange=\"selectLDevice(this);\" id=\"selDev\">");
  
  static const char scripts[] PROGMEM = R"=====(
    <script>
    function setDeviceType(elem, deviceId) {
      sendGet( "\light?sdevice=" + deviceId  + "&tdevice=" + elem[elem.selectedIndex].value );
    };
    function setDeviceOperation(elem) {
      
    };
    function bindLDevice(deviceId) {
      sendGet( "\light?sdevice=" + deviceId  + "&lstate=" + 2 );
    };
    function unbindLDevice(deviceId) {
      sendGet( "\light?sdevice=" + deviceId  + "&lstate=" + 1 );
    };
    function removeDevice() {
      sendGet( "\light?rdevice=true" );
    };
    function addDevice() {
      sendGet( "\light?adevice=true" );
      location.reload(true);
    };
    function addSignal() {
      var list = document.getElementById('selDev');
      var sig_list = document.getElementById('selSig');
      var pt_list = document.getElementById('selPt');
      var indx = list.selectedIndex;
      var sig_indx = sig_list.selectedIndex;
      var pt_indx = pt_list.selectedIndex;
      
      sendGet( "\light?sigdev=" + list[indx].value + "&asigid=" + sig_list[sig_indx].value + "&asigpt=" + pt_list[pt_indx].value + "&asigxy=" +  (document.getElementById("chXy").checked? "1" : "0") + "&asigvalue=" + document.getElementById("inVal").value );
      location.reload(true);
    };
    function addPDevice() {
      sendGet( "\power?addpdevice=true" );
      location.reload(true);
    };
    function setPDeviceState(id, state) {
      sendGet( "\power?pdevice=" + id + "&pstate=" + state );
    };
    function bindPDevice(id) {
      sendGet("\power?pdevice=" + id + "&pstate=" + 2);
      if (confirm("Are you sure that your power outlet has been binded? Accept this check box if your power socket has been looked."))
      {
        sendGet("\power?pdevice=" + id + "&pstate=" + 1);
      }
      else
      {
        sendGet("\power?pdevice=" + id + "&pstate=" + 0);
      }
    };
    function unbindPDevice(id) {
      sendGet("\power?pdevice=" + id + "&pstate=" + 3);
      if (confirm("Are you sure that your power outlet has been unbinded? WARNING!! Any socket registered with an unkwnown code should not be binded anymore."))
      {
        sendGet("\power?pdevice=" + id + "&pstate=" + 0);
      }
      else
      {
        sendGet("\power?pdevice=" + id + "&pstate=" + 0);
      }
    };
    function changePSettings(pid, sid, el){
      //alert(pid + " " + v + " " + el.value);
      sendGet("\power?pdevice=" + pid + "&psid=" + sid + "&pvalue=" + el.value);
    };
    function setReboot()
    {
      sendGet("\global?setreboot=" + 1);
    }
    function setFactory()
    {
      if (confirm("This action will set default settings. Are you sure ?"))
      {
        sendGet("\global?setfactorysettings=" + 1);
      }
    }
    function setPermanent()
    {
      store();
    }
    function changePDescription(pid, el)
    {
      sendGet("\power?pdevice=" + pid + "&pdesc=" + el.value);
    }
    </script>
  )=====";

  *str += FPSTR(&scripts[0]);
}

void AcHtml::gen_listbox_lightdevices(AcStorage * const lstorage, String * const str)
{
  // list devices to select
  for(int i=0; i < lstorage->getNLDevice(); i++)
  {
    *str += F("<option ");
    if ( i == lstorage->getCurrentSelLDevice() )
      *str += F("selected ");
    *str += F("value=\"");
    *str += String( lstorage->getLDeviceId(i) );
    *str += F("\">DEVICE ");
    *str += String(lstorage->getLDeviceId(i));
    *str += F("</option>");
  }
}

void AcHtml::get_advset_light_devicesel(AcStorage * const lstorage, String * const str)
{
  gen_listbox_lightdevices(lstorage, str);

  if ( lstorage->getNLDevice() )
    *str +=  F("</select><table style=\"width:100%;border: 1px solid black;border-collapse: collapse;\">");
}

void AcHtml::get_advset_light_device_signals(AcStorage * const lstorage, String* s)
{ 
  if(!lstorage->getNLDevice())
    return;
  
  // get selected device
  for (int i=0; i < N_SIGNALS; i++)
  {
    *s += "<tr>";
    for (int j=0; j < S_LEN_EACH-1; j+=2)
    {
      *s += "<td>" + String(lstorage->getLDeviceSignal(i,j)) + "," + String(lstorage->getLDeviceSignal(i,j+1)) + "</td>";
    }
    *s += "<td>" + String(lstorage->getLDeviceSignal(i,S_LEN_EACH-1)) + "</td></tr>";
  }
}

void iterate_option(String * str, unsigned int condition)
{
  for (int i=0; i < condition; i++)
  {
    *str += F("<option value=\"");
    *str += String(i+1);
    *str += F("\">");
    *str += String(i+1);
    *str += F("</option>");
  }
}

void AcHtml::get_advset_light4(AcStorage * const lstorage, String *str)
{
  if ( !lstorage->getNLDevice() )
  {
     *str += F("</div>");
     return; 
  }
  
  *str += F("</table>Line:<select id=\"selSig\">");
  iterate_option(str, N_SIGNALS);
  *str += F("</select> Column:<select id=\"selPt\">");
  iterate_option(str, ((S_LEN_EACH-1)/2) + 1);
  *str += F("</select> Val:<input text id=\"inVal\"></input text><input type=\"checkbox\" id=\"chXy\"/><button id=\"asignal\" onclick=\"addSignal();\">Set</button></div>");
}
 
void AcHtml::get_advset_clock(String *str, const RtcDateTime clock)
{
  static const char clockbegin[] PROGMEM = R"=====(
<h2><a href="#" onclick="toggle('wrapper2')">Real-time clock</a></h2>
<div id="wrapper2" class="wrapper" style="display:none">
<table style="width:100%;border: 1px solid black;border-collapse: collapse;">
<tr><td>Hour</td><td>Minutes</td><td>Seconds</td><td>Day</td><td>Month</td><td>Year</td></tr>
<tr>
  )=====";

  static const char clockend[] PROGMEM = R"=====(
</tr></table>
<script>
function setClock() {
  var xmlHttp = new XMLHttpRequest();
  var list = document.getElementById('selTclock');
  var indx = list.selectedIndex;
  var str = "";
  if (list[indx].value == 1)
    str += "stimeh";
  else
  {
    if (list[indx].value == 2)
      str += "stimem";
    else
    {
      if (list[indx].value == 3)
        str += "stimed";
      else
      {
        if (list[indx].value == 4)
          str += "stimemo";
        else
        {
          if (list[indx].value == 5)
            str += "stimey";
          else
          {
            return;
          }
        }
      }
    }
  }
  xmlHttp.open( "GET", "\clock?" + str + "=" + document.getElementById('clockval').value);
  xmlHttp.send( null );
};
</script>
<select id="selTclock">
  <option value="1">Hour</option>
  <option value="2">Minutes</option>
  <option value="3">Day</option>
  <option value="4">Month</option>
  <option value="5">Year</option>
</select>
Val:<input text id="clockval"></input text><button onclick="setClock();">Set</button>
</div>
  )=====";
 
  *str += FPSTR(&clockbegin[0]);

  *str += F("<td>");
  *str += String(clock.Hour());
  *str += F("</td><td>");
  *str += String(clock.Minute());
  *str += F("</td><td>");
  *str += String(clock.Second());
  *str += F("</td><td>");
  *str += String(clock.Day());
  *str += F("</td><td>");
  *str += String(clock.Month());
  *str += F("</td><td>");
  *str += String(clock.Year());
  *str += F("</td>");
  
  *str += FPSTR(&clockend[0]);
}

void AcHtml::get_advset_psockets(String *str, unsigned int n_powerDevices, AcStorage::deviceDescriptorPW* pdevice)
{
  char tmp[12];
  
  static const char psockets[] PROGMEM =
  R"=====(
  <h2><a href="#" onclick="toggle('wrapper3')">Power Sockets</a></h2>
  <div id="wrapper3" class="wrapper" style="display:none">
  <table style="width:100%;border: 1px solid black;border-collapse: collapse;">
  <tr>
    <td>ID</td>
    <td>Description</td>
    <td>CodeID</td>
    <td>Devide Id</td>
    <td></td>
    <td>State</td>
    <td></td>
    <td></td>
  </tr>
  )=====";

  *str += FPSTR(&psockets[0]);

  // openfile to get settings for power outlets
  File psocketsFile = SPIFFS.open("/psocket_settings.txt", "r");
  uint8_t buf[31];
  
  // print power devices
  for (int i=0; i < n_powerDevices; i++)
  {
    *str += F("<tr>");
    
    *str += F("<td>");
    *str += String(pdevice[i].id);
    *str += F("</td>");

    *str += F("<td><input type=\"text\" value=\"");
    psocketsFile.read( buf, 30 );
    String str_tmp = String( (char*)buf );
    str_tmp.replace( "%20", " " );
    *str += str_tmp;
    *str += F("\" onchange=\"changePDescription(");
    *str += String( idx2id( i ) );
    *str += F(", this)\" class=\"inputp\" /></td>");

    
    *str += F("<td>0x");
    sprintf(tmp,"%07x", pdevice[i].code);
    *str += String(tmp);
    *str += F("</td>");
  
    if (pdevice[i].type == CHACON_DIO)
    {
      *str += F("<td>CHACON_DIO</td>");
    }
    else
    {
      *str += F("<td>OTHER</td>");
    }

    *str += F("<td><button onclick=\"setPDeviceState(");
    *str += String(pdevice[i].id);
    *str += F(",4);\">On</button> <button onclick=\"setPDeviceState(");
    *str += String(pdevice[i].id);
    *str += F(",5);\">Off</button> ");
    *str += F("<button onclick=\"setPDeviceState(");
    *str += String(pdevice[i].id);
    *str += F(",6);\">AUTO</button></td>");

    *str += F("<td>");
    *str += (pdevice[i].state == ON ) ? F("ON") :
    ( (pdevice[i].state == OFF ) ? F("OFF") :
    ( (pdevice[i].state == BINDING ) ? F("BINDING") :
    ( (pdevice[i].state == UNBINDING ) ? F("UNBINDING") :
    ( (pdevice[i].state == ON_PERMANENT ) ? F("PERMANENT ON") :
    ( (pdevice[i].state == OFF_PERMANENT ) ? F("PERMANENT OFF") :
    (F(".")) ) ) ) ) );
    *str += F("</td>");
    
    *str += F("<td><button  onclick=\"bindPDevice(");
    *str += String(pdevice[i].id);
    *str += F(");\">Bind</button> <button  onclick=\"unbindPDevice(");
    *str += String(pdevice[i].id);
    *str += F(");\">Unbind</button></td>");
    
    *str += F("<td><button>Remove</button></td>");
    
    *str += F("</tr>");
  }

  // close file stream
  psocketsFile.close();

  static const char psockets2[] PROGMEM =
  R"=====(
  </table><button  onclick="addPDevice();">Add Device</button>
  )=====";

  *str += FPSTR(&psockets2[0]);
}

void AcHtml::get_advset_psockets_step( AcStorage * const lstorage, String * const str , ESP8266WebServer * server, std::function<void (String*)> sendBlock )
{ 
  char tmp[10];
  *str += F("<h3>Power Outlet Activation Signals (any cell check is discarded)</h3><style>.tdp {border: 1px solid gray;border-collapse: collapse;} .inputp {width:100%; background: transparent; border: none; text-align:center;}</style><table style=\"width:100%;border: 1px solid black;border-collapse: collapse;\"><tr><th colspan=\"2\">a1</th><th colspan=\"2\">b1</th><th colspan=\"2\">c1</th><th colspan=\"2\">d1</th><th colspan=\"2\">a2</th><th colspan=\"2\">b2</th><th colspan=\"2\">c2</th><th colspan=\"2\">d2</th><th colspan=\"2\">a3</th><th colspan=\"2\">b3</th><th colspan=\"2\">c3</th><th colspan=\"2\">d3</th><th colspan=\"2\">a4</th><th colspan=\"2\">b4</th><th colspan=\"2\">c4</th><th colspan=\"2\">d4</th><th>Size</th></tr>");

  for (int i=0; i < lstorage->getNumberOfPowerDevices(); i++)
  {
    *str += F("<tr>");

      for (int j=0; j < S_LEN_EACH ; j+=2)
      {
        *str += F("<td class=\"tdp\"><input type=\"text\" value=\"");
        *str += String( lstorage->getPDeviceStep(i, j) );
        if( j != S_LEN_EACH-1 )
        {
          *str += F("\" onchange=\"changePSettings(");
          
          sprintf(tmp, "%i,%i", idx2id(i), idx2id(j) );
          *str += String( tmp );
          
          *str += F(",this)\" class=\"inputp\" /></td>");
          *str += F("<td class=\"tdp\"><input type=\"text\" value=\"");
          *str += String( lstorage->getPDeviceStep(i, j+1) );
        }
        
        *str += F("\" onchange=\"changePSettings(");
        
        sprintf(tmp, "%i,%i", idx2id(i),  (j != S_LEN_EACH-1) ? idx2id(j+1) : idx2id(j) );
        *str += String( tmp );
        
        *str += F(",this)\" class=\"inputp\" /></td>");
      }
      
    *str += F("</tr>");

    sendBlock(str);
  }

  *str += F("</table></div><button onClick=\"setReboot()\">Reboot</button> <button onClick=\"setFactory()\">Factory Settings</button> <button onClick=\"setPermanent()\">Set Settings Permanent</button><br></div>");
  
}

void AcHtml::send_status_div(String * const str, AcStorage * const lstorage, std::function<void (String*)> sendBlock )
{

  static const char ss_tmp[] PROGMEM = R"=====(
<style>
.setting {
  position:relative;
  margin-bottom:.453em;
  line-height:1.993em;
  width:100%;
}
.setting .label {
  float:left;
  width:18.931em;
}
fieldset .setting .label {
  width:18.116em;
}
.setting .default {
  display:block;
  position:absolute;
  color:#666;
  top:0;
  right:0;
  width:16.304em;
}
fieldset {
  border:solid .09em #ccc;
  padding:.725em;
  margin:0;
}
fieldset legend {
  font-weight:bold;
  margin-left:-.362em;
  padding:0 .272em;
  background:#fff;
}
</style>

<div class="settings" id="settings"><h2><a href="#">System Information</a></h2>
  <fieldset>
<legend>Controller</legend>
<div class="setting">
  <div class="label">Model</div>
)=====";

  *str += FPSTR(&ss_tmp[0]);
  sendBlock(str);

  *str += F("Opaq v");
  *str += String( OPAQ_VERSION );
  *str += String(" C");
  *str += String( ESP.getFlashChipId() );

  *str += F("</div><div class=\"setting\"><div class=\"label\">Wireless MAC</div><span id=\"wl_mac\" style=\"cursor:pointer; text-decoration:underline;\" title=\"OUI Search\">");

  // sample: 00:14:BF:3B:F8:DD
  *str += WiFi.softAPmacAddress();

  *str += F("</span></div></fieldset><br>");

  *str += F("<fieldset><legend>Wireless</legend><div class=\"setting\"><div class=\"label\">Radio Time Restrictions</div><span id=\"wl_radio\">Radio is On</span>&nbsp;</div><div class=\"setting\"><div class=\"label\">Mode</div>");

  if ( lstorage->getModeOperation() )
  {
    *str += F("softAP");
  }
  else
  {
    *str += F("client");
  }

  *str += F("</div><div class=\"setting\"><div class=\"label\">SSID</div>");

  // sample: opaq-AAAA
  *str += String( lstorage->getSSID() );

  *str += F("</div><div class=\"setting\"><div class=\"label\">DHCP Server</div>Enabled&nbsp;</div><div class=\"setting\"><div class=\"label\">Channel</div><span id=\"wl_channel\">");

  // sample: 6
  *str += String ( WiFi.channel() );

  *str += F("</span>&nbsp;</div></fieldset><br>");


  *str += F("</div>");
  sendBlock(str);
}

