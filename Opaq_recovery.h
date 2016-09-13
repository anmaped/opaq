

// Opaq_recovery.h

#include <ESPAsyncWebServer.h>


const char index_html[] PROGMEM = R"(
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>Recovery Page</title>
</head>
<style>body{background:#fffff;font-family:"Lucida Grande", Tahoma, Arial, Verdana, sans-serif;font-size:small;margin:8px 0 16px;text-align:center;}
#form_container{background:#fff;border:1px solid #ccc;margin:0 auto;text-align:left;width:640px;}
#top{display:block;height:10px;margin:10px auto 0;width:650px;}
#footer{width:640px;clear:both;color:#999999;text-align:center;width:640px;padding-bottom:15px;font-size:85%;}
#footer a{color:#999999;text-decoration:none;border-bottom:1px dotted #999999;}
#bottom{display:block;height:10px;margin:0 auto;width:650px;}
form.appnitro{margin:20px 20px 0;padding:0 0 20px;}
h1{background-color:#dedede;margin:0;min-height:0;padding:0;text-decoration:none;}
h1 a{display:block;height:100%;min-height:40px;overflow:hidden;}
img{behavior:url(css/iepngfix.htc);border:none;}
.appnitro{font-family:Lucida Grande, Tahoma, Arial, Verdana, sans-serif;font-size:small;}
.appnitro li{width:61%;}
form ul{font-size:100%;list-style-type:none;margin:0;padding:0;width:100%;}
form li{display:block;margin:0;padding:4px 5px 2px 9px;position:relative;}
form li:after{clear:both;content:".";display:block;height:0;visibility:hidden;}
.buttons:after{clear:both;content:".";display:block;height:0;visibility:hidden;}
.buttons{clear:both;display:block;margin-top:10px;}
* html form li{height:1%;}
* html .buttons{height:1%;}
* html form li div{display:inline-block;}
form li div{color:#444;margin:0 4px 0 0;padding:0 0 8px;}
form li span{color:#444;float:left;margin:0 4px 0 0;padding:0 0 8px;}
form li div.left{display:inline;float:left;width:48%;}
form li div.right{display:inline;float:right;width:48%;}
form li div.left .medium{width:100%;}
form li div.right .medium{width:100%;}
.clear{clear:both;}
form li div label{clear:both;color:#444;display:block;font-size:9px;line-height:9px;margin:0;padding-top:3px;}
form li span label{clear:both;color:#444;display:block;font-size:9px;line-height:9px;margin:0;padding-top:3px;}
form li .datepicker{cursor:pointer !important;float:left;height:16px;margin:.1em 5px 0 0;padding:0;width:16px;}
.form_description{clear:both;display:inline-block;margin:0 0 1em;}
.form_description[class]{display:block;}
.form_description h2{clear:left;font-size:160%;font-weight:400;margin:0 0 3px;}
.form_description p{font-size:95%;line-height:130%;margin:0 0 12px;}
form hr{display:none;}
form li.section_break{border-top:1px dotted #ccc;margin-top:9px;padding-bottom:0;padding-left:9px;padding-top:13px;width:97% !important;}
form ul li.first{border-top:none !important;margin-top:0 !important;padding-top:0 !important;}
form .section_break h3{font-size:110%;font-weight:400;line-height:130%;margin:0 0 2px;}
form .section_break p{font-size:85%;margin:0 0 10px;}
input.button_text{overflow:visible;padding:0 7px;width:auto;}
.buttons input{font-size:120%;margin-right:5px;}
label.description{border:none;color:#222;display:block;font-size:95%;font-weight:700;line-height:150%;padding:0 0 1px;}
span.symbol{font-size:115%;line-height:130%;}
input.text{background:#fff url(../../../images/shadow.gif) repeat-x top;border-bottom:1px solid #ddd;border-left:1px solid #c3c3c3;border-right:1px solid #c3c3c3;border-top:1px solid #7c7c7c;color:#333;font-size:100%;margin:0;padding:2px 0;}
input.file{color:#333;font-size:100%;margin:0;padding:2px 0;}
textarea.textarea{background:#fff url(../../../images/shadow.gif) repeat-x top;border-bottom:1px solid #ddd;border-left:1px solid #c3c3c3;border-right:1px solid #c3c3c3;border-top:1px solid #7c7c7c;color:#333;font-family:"Lucida Grande", Tahoma, Arial, Verdana, sans-serif;font-size:100%;margin:0;width:99%;}
select.select{color:#333;font-size:100%;margin:1px 0;padding:1px 0 0;background:#fff url(../../../images/shadow.gif) repeat-x top;border-bottom:1px solid #ddd;border-left:1px solid #c3c3c3;border-right:1px solid #c3c3c3;border-top:1px solid #7c7c7c;}
input.currency{text-align:right;}
input.checkbox{display:block;height:13px;line-height:1.4em;margin:6px 0 0 3px;width:13px;}
input.radio{display:block;height:13px;line-height:1.4em;margin:6px 0 0 3px;width:13px;}
label.choice{color:#444;display:block;font-size:100%;line-height:1.4em;margin:-1.55em 0 0 25px;padding:4px 0 5px;width:90%;}
select.select[class]{margin:0;padding:1px 0;}
*:first-child+html select.select[class]{margin:1px 0;}
.safari select.select{font-size:120% !important;margin-bottom:1px;}
input.small{width:25%;}
select.small{width:25%;}
input.medium{width:50%;}
select.medium{width:50%;}
input.large{width:99%;}
select.large{width:100%;}
textarea.small{height:5.5em;}
textarea.medium{height:10em;}
textarea.large{height:20em;}
#error_message{background:#fff;border:1px dotted red;margin-bottom:1em;padding-left:0;padding-right:0;padding-top:4px;text-align:center;width:99%;}
#error_message_title{color:#DF0000;font-size:125%;margin:7px 0 5px;padding:0;}
#error_message_desc{color:#000;font-size:100%;margin:0 0 .8em;}
#error_message_desc strong{background-color:#FFDFDF;color:red;padding:2px 3px;}
form li.error{background-color:#FFDFDF !important;border-bottom:1px solid #EACBCC;border-right:1px solid #EACBCC;margin:3px 0;}
form li.error label{color:#DF0000 !important;}
form p.error{clear:both;color:red;font-size:10px;font-weight:700;margin:0 0 5px;}
form .required{color:red;float:none;font-weight:700;}

/* Center the loader */
#loader {
  position: absolute;
  left: 50%;
  top: 50%;
  z-index: 1;
  width: 150px;
  height: 150px;
  margin: -75px 0 0 -75px;
  border: 16px solid #f3f3f3;
  border-radius: 50%;
  border-top: 16px solid #3498db;
  width: 120px;
  height: 120px;
  -webkit-animation: spin 2s linear infinite;
  animation: spin 2s linear infinite;
}

@-webkit-keyframes spin {
  0% { -webkit-transform: rotate(0deg); }
  100% { -webkit-transform: rotate(360deg); }
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

</style>
<script language=javascript>
function getUrlVars() {
var vars = {};
var parts = window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function(m,key,value) {
vars[key] = value;
});
return vars;
}

(function() {
//alert(getUrlVars()["success"])
if(getUrlVars()["success"] == "true")
{
	alert("Upload has been done successfully. Please wait while opaq is unarchiving the filesystem. It may take a while!");
}
})();

function validate(){
  
  var c = document.getElementById('formatspiffscheck');

  document.getElementById('WaitDialog').style.display = 'inline';
  document.getElementById('loader').style.display = 'inline';

  if (c.checked) {
  var xmlHttp = new XMLHttpRequest();
  xmlHttp.open( "GET", "http://" + window.location.hostname + "/formatspiffs", false ); // false for synchronous request
  xmlHttp.send( null );
  //alert(xmlHttp.responseText);

  //alert("formating! Please take a while!");
  

  setTimeout(function()
  {
    document.getElementById('form1').submit();
    document.getElementById('WaitDialog').style.display = 'none';
    document.getElementById('loader').style.display = 'none';
  }, 60000);
  }
  else
  {
  	return true;
  }

  return false;
}
</script>
<body id="main_body">
<div id="form_container">
<h1>Recovery Page</h1>
<p style="padding:10px"><font color="#ff3333">WARNING!!</font> The opaq project is not responsible for any faulty update or damage of the device. DO THAT AT YOUR OWN RISK. Please be carefull. We recomend you to avoid using this recovery mode if you are not aware of the task you are performing.</p>
<form id="form1" class="appnitro" onsubmit="return validate() " enctype="multipart/form-data" method="POST" action="/upload">
<div class="form_description">
<h2>Updating Filesystem</h2>
<p>This option allows us to update the filesystem with the lastest web interface. Older versions are not allowed.</p>
</div>
<ul>
<li class="section_break">
</li>
<li id="li_1">
<label class="description" for="element_1">Upload a File </label>
<div>
<input id="element_1" name="upload" class="element file" type="file"/>
</div>
</li>
<li id="li_5">
<label class="description" for="element_5">Options: </label>
<span>
<input id="formatspiffscheck" name="element_5_1" class="element checkbox" type="checkbox" value="1" />
<label class="choice" for="formatspiffscheck">Format SPIFFS</label>
</span>
</li>
<li class="buttons">
<input id="saveForm" class="button_text" type="submit" name="submitt" value="upload"/>
</li>
</ul>
</form>
<form id="form2" class="appnitro" enctype="multipart/form-data" method="POST" action="/upload">
<div class="form_description">
<h2>Flashing Firmware</h2>
<p>OTA update is available with this option.</p>
</div>
<ul>
<li class="section_break">
</li>
<li id="li_4">
<label class="description" for="element_4">Upload a File </label>
<div>
<input id="element_4" name="element_4" class="element file" type="file"/>
</div>
</li>
<li class="buttons">
<input type="hidden" name="form_id" value="1155571"/>
<input id="saveForm" class="button_text" type="submit" name="submitt" value="Flash"/>
</li>
</ul>
</form>
<div id="footer">
</div>
</div>

<div id="loader" style="display:none;"></div>

<div id="WaitDialog" style="text-align: center;display:none;">
  
  <div style="margin-top: 10px; color: black">
    <b>Please wait</b>
  </div>
</div>

</body>
</html>
)";

void opaq_recovery(AsyncWebServerRequest * request)
{
	request->send_P(200, "text/html", index_html);
}
