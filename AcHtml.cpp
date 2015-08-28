
#include "AcHtml.h"

//#include "flash.h"
//#include "websites.h"

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
    <title>Aqua Controller</title> \
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
    <title>Aqua Controller</title> \
    <meta charset=\"utf-8\" /> \
    <link rel=\"stylesheet\" href=\"style.css\"> \
    </head> ");
}

const __FlashStringHelper* AcHtml::get_menu()
{
  static const char ss[] PROGMEM = (R"=====(
  <img height="100px" src="logo.png"/>
  <nav class="dropdownmenu">
  <ul>
    <li><a href="..">Status</a></li><!--
    --><li><a href="#">Settings</a>
    <ul id="submenu">
        <li><a href="light">Light</a></li>
        <li><a href="co2">CO2</a></li>
        <li><a href="advset">Advanced Settings</a></li>
      </ul>
    </li><!--
    --><li><a href="info">Info</a></li>
  </ul>
</nav>
  )=====");
  
  return FPSTR(&ss[0]);
}

const __FlashStringHelper* AcHtml::get_body_begin()
{
  return F(" <body> ");
}

const __FlashStringHelper* AcHtml::get_body_end()
{
  return F(" </body> ");
}

void AcHtml::get_light_script(String * str, unsigned int n_ldevices, AcStorage::deviceLightDescriptor *device)
{
  static const char script_chart[] PROGMEM = R"=====(
  <canvas class="lightsignals" id="light"></canvas>
  <font size="4">Light device: </font><select><option>1</option></select> <button>Apply Settings</button>
  <script>
    var buyerData = {
    labels : ["0am", "4am","8am","12am","4pm","8pm","12pm"],
    labelsspace : 4,
  )=====";

  static const char script_chart2[] PROGMEM = R"=====(
    ]
  }		
  var buyers = document.getElementById('light').getContext('2d');
  new Chart(buyers).Line(buyerData);
  </script>
  )=====";
  
  *str += FPSTR(&script_chart[0]);
  *str += "device : " + String(n_ldevices) + ",";
  *str += F("datasets : [");
  /*  {
        fillColor : "rgba(172,194,132,0)",
        strokeColor : "#ACC26D",
        pointColor : "#fff",
        pointStrokeColor : "#9DB86D",
        data  : [[3,0],[15,5],[30,12],[80,16],[50,20],[3,24]],
      },
      {
        fillColor : "rgba(172,194,132,0)",
        strokeColor : "#ACC200",
        pointColor : "#fff",
        pointStrokeColor : "#90086D",
        data : [[15,0],[20,7],[30,10],[90,14],[50,20],[15,24]]
      }*/
  if (n_ldevices > 0)
  {
  for(int s=0; s < N_SIGNALS; s++)
  {
    int lend = device[n_ldevices-1].signal[s][S_LEN_EACH-1];
    if (lend != 0)
    {
      *str += "{";

      *str += "fillColor : \"rgba(172,194,132,0)\",";
      *str += "strokeColor : \"#ACC200\",";
      *str += "pointColor : \"#fff\",";
      *str += "pointStrokeColor : \"#90086D\",";
      *str += "data : [";
      for (int i=0; i < lend ; i++)
      {
        *str += "[";
        *str += String(device[n_ldevices-1].signal[s][i*2]);
        *str += ",";
        *str += String(device[n_ldevices-1].signal[s][i*2+1]);
        *str += ",[";
        *str += String(s+1);
        *str += ",";
        *str += String(i+1);
        *str += "]],";
      }
      *str += "]},";
    }
  }
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
  *str += F("<div class=\"settings\" id=\"settings\"><h2><a href=\"#\" onclick=\"toggle('wrapper')\">Light Controller</a></h2><div id=\"wrapper\" class=\"open\" style=\"display:none\">Devices:<table style=\"width:100%;border: 1px solid black;border-collapse: collapse;\"><tr><td>Id</td><td>Type</td><td>CodeID</td><td>Linear</td><td></td><td></td></tr>");
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
  // get registerd devices

  //String a = "";
  
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
    //a+=String(device[i].codeid);
    *a += "X";
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
    
    *a += F("<td><button onclick=\"bindDevice();\">Bind</button></td><td>");
    if ( i == n_ldevices-1)
    {
      *a += F("<button onclick=\"removeDevice();\">Remove</button>");
    }
    *a += F("</td></tr>");
  }
  
  
  //return a;
}


void AcHtml::get_advset_light2(String *str)
{
  *str += F("</table><input id=\"adevice\" type=\"button\" value=\"Add Device\" onclick=\"addDevice();\" />");
  *str += F("<br><br><h3>Light Settings (Points of lines or splines)</h3>Device Selected: <select id=\"selDev\">");
  
  static const char scripts[] PROGMEM = R"=====(
    <script>
    function setDeviceType(elem, deviceId) {
      var xmlHttp = new XMLHttpRequest();
      xmlHttp.open( "GET", "\light?sdevice=" + deviceId  + "&tdevice=" + elem[elem.selectedIndex].value, false );
      xmlHttp.send( null );
    };
    function setDeviceOperation(elem) {
      
    };
    function bindDevice() {
      
    };
    function removeDevice() {
      var xmlHttp = new XMLHttpRequest();
      xmlHttp.open( "GET", "\light?rdevice=true", false );
      xmlHttp.send( null );
    };
    function addDevice() {
      var xmlHttp = new XMLHttpRequest();
      xmlHttp.open( "GET", "\light?adevice=true", false );
      xmlHttp.send( null );
      location.reload(true);
    };
    function addSignal() {
      var xmlHttp = new XMLHttpRequest();
      
      var list = document.getElementById('selDev');
      var sig_list = document.getElementById('selSig');
      var pt_list = document.getElementById('selPt');
      var indx = list.selectedIndex;
      var sig_indx = sig_list.selectedIndex;
      var pt_indx = pt_list.selectedIndex;
      
      xmlHttp.open( "GET", "\light?sigdev=" + list[indx].value + "&asigid=" + sig_list[sig_indx].value + "&asigpt=" + pt_list[pt_indx].value + "&asigxy=" +  (document.getElementById("chXy").checked? "1" : "0") + "&asigvalue=" + document.getElementById("inVal").value, false );
      xmlHttp.send( null );
      location.reload(true);
    };
    </script>
  )=====";

  *str += FPSTR(&scripts[0]);
}

void AcHtml::get_advset_light_devicesel(unsigned int n_ldevices, AcStorage::deviceLightDescriptor *device, String* a)
{
  //String a = "";
  
  // list devices to select
  for(int i=0; i < n_ldevices; i++)
  {
    *a += "<option value=\"" + String(device[i].id) + "\">" + String(device[i].id) + "</option>";
  }
  
  //return a;
}

const __FlashStringHelper* AcHtml::get_advset_light3()
{
  return F("</select><table style=\"width:100%;border: 1px solid black;border-collapse: collapse;\">");
}

void AcHtml::get_advset_light_device_signals(unsigned int n_ldevices, AcStorage::deviceLightDescriptor *device, String* s)
{
  //String s = "";
  
  if(n_ldevices==0)
    return;
  
  // get selected device
  for (int i=0; i < N_SIGNALS; i++)
  {
    *s += "<tr>";
    for (int j=0; j < S_LEN_EACH-1; j+=2)
    {
      *s += "<td>" + String(device[n_ldevices-1].signal[i][j]) + "," + String(device[n_ldevices-1].signal[i][j+1]) + "</td>";
    }
    *s += "<td>" + String(device[n_ldevices-1].signal[i][S_LEN_EACH-1]) + "</td></tr>";
  }
  
  //return s;
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

void AcHtml::get_advset_light4(String *str)
{
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
<div id="wrapper2" class="open" style="display:none">
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

void AcHtml::get_advset_psockets(String *str)
{
  static const char psockets[] PROGMEM = R"=====(
  <h2><a href="#" onclick="toggle('wrapper3')">Power Sockets</a></h2>
<div id="wrapper3" class="open" style="display:none">
<table style="width:100%;border: 1px solid black;border-collapse: collapse;">
  <tr>
    <td>ID</td>
    <td>CodeID</td>
    <td>Devide</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>1</td>
    <td>00111101111110001011111110</td>
    <td>CHANON_DIO</td>
    <td><button>Bind</button></td>
    <td><button>Remove</button></td>
  </tr>
  <tr>
    <td>2</td>
    <td>00111101111110001011111110</td>
    <td>CHANON_DIO</td>
    <td><button>Bind</button></td>
    <td><button>Remove</button></td>
  </tr>
  </table>
    <button>Add Device</button>
    
  </div>
  <br>
  </div>
  )=====";

   *str += FPSTR(&psockets[0]);
}

