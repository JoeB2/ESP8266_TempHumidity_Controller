extern const char INDEX_HTML[] PROGMEM =  R"=====(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: left;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 2.5rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
    input[type=submit] {
      border: 0;
      line-height: 2.5;
      padding: 0 5px;
      font-size: 17px;
      text-align: center;
      
      font-weight: bolder;
      padding: 3px 3x;
      text-decoration: none;
      margin: 0px 0px;
      cursor: pointer;
      width: auto;
      box-shadow: inset 2px 2px 3px rgba(255, 255, 255, .6),
                  inset -2px -2px 3px rgba(0, 0, 0, .6);
   }
  </style>
  <title>Temp/Humid monitor & Switch Device</title>
</head>
  <input id="help" type="submit" value="Help" style="font-size: 14px;line-height: 1.5;" onclick="f_help()">
  <h2>ESP8266 DHT Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature1</span>
    <span id="t1">%T1%</span>
    <sup class="units">&deg;F</sup>
    <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="dht-labels">Humidity1</span>
    <span id="h1">%H1%</span>
    <sup class="units">%</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature2</span> 
    <span id="t2">%T2%</span>
    <sup class="units">&deg;F</sup>
    <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="dht-labels">Humidity2</span>
    <span id="h2">%H2%</span>
    <sup class="units">%</sup>
  </p>
  <table>
    <tr>
      <td style="width: 60px; font-weight: bolder;">Heat  1</td>
      <td style="width: 70px; font-weight: bolder;">Humid 1</td>
      <td style="width: 60px; font-weight: bolder;">Heat  2</td>
      <td style="width: 70px; font-weight: bolder;">Humid 2</td>
    </tr>
    <tbody>
      <tr>
        <td id="heat1"><i class="fas fa-toggle-on" style="color: green;"></i>on</td>
        <td id="humid1"><i class="fas fa-toggle-off" style="color:red;"></i>off</td>
        <td id="heat2"><i class="fas fa-toggle-off" style="color:red;"></i>off</td>
        <td id="humid2"><i class="fas fa-toggle-off" style="color:red;"></i>off</td>
      </tr>
     </tbody>
  </table>
  <br><br>
  <input value="Set Limits" type="submit" onclick="f_redirect('Set_Bounds')" style="background:moccasin">
  <p id="message1" style="color:gray;font-size:13px;font-weight:bold" hidden>TEST</p>
</body>

  <script language = "javascript" type = "text/javascript">

    let webSock       = new WebSocket('ws://'+window.location.hostname+':80');
    webSock.onopen    = function(evt){webSock.send("Connect");}
    webSock.onmessage = function(evt){f_webSockOnMessage(evt);}
    webSock.onerror   = function(evt){f_webSockOnError(evt);}

    function f_webSockOnMessage(evt){
      if(typeof evt.data === "string"){
        document.getElementById("message1").innerHTML = evt.data;
        var t='<i class="fas fa-toggle-on" style="color: green;"></i>on';
        var f='<i class="fas fa-toggle-off" style="color:red;"></i>off';
        console.log(evt.data);
        j=JSON.parse(evt.data);
          for(var key in j[0])
              if(j[0].hasOwnProperty(key)){
                console.log(key);
                if(key=="heat1")document.getElementById(key).innerHTML=j[0][key]?t:f;else
                if(key=="humid1")document.getElementById(key).innerHTML=j[0][key]?t:f;else
                if(key=="heat2")document.getElementById(key).innerHTML=j[0][key]?t:f;else
                if(key=="humid2")document.getElementById(key).innerHTML=j[0][key]?t:f;else
                document.getElementById(key).innerHTML=j[0][key]==-999?"n/a":j[0][key];
              }
        document.getElementById("message1").innerHTML=evt.data;
      }
    }
    function f_webSockOnError(evt){}
    function f_help(){
      j = document.getElementById("json");
      let v = document.getElementById("help");
      document.getElementById("message1").hidden = v.value=="Help"?false:true;
      v.value=v.value=="Help"?"Un-Help":"Help";
    }
    function f_redirect(where){;
                      window.location.href = "/"+where;
    }
  </script>
</html>
)=====";

extern const char SET_BOUNDS_HTML[] PROGMEM =  R"=====(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta name="file" content="set_bounds.html">
    <meta name="author" content="Joe Belson 20210822">
    <meta name="what" content="esp8266 html for WiFi connected temp/humidity bounds.">
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Set Temp / Humidity Bounds for 110VAC relays</title>
  </head>
<style type="text/css">
      body {font-size: 12px; background-color: #f6f6ff;font-family: Calibri, Myriad;text-align: center;}
      body p{text-align: left;}
      table th{text-align: center;width: 23pt;border-bottom:2px solid rgb(8, 8, 8);}
      table.calibrate {font-size:smaller ; margin-left:10px; margin-right: auto;  width: 300px;  border: 1px solid black; border-style:solid;}
      table.calibrate caption {	background-color: #f79646;	color: #fff;	font-size: large;	font-weight: bold;	letter-spacing: .3em;}
      table.calibrate thead th {
                  padding: 3px;
                  background-color: #fde9d9;
                  font-size:16px;
                  border-width: 1px;
                  border-style: solid;
                  border-color: #f79646 #ccc;
      }
      table.calibrate td {
                  font-size: 12px;
                  font-weight: bold;
                  text-align: center;
                  padding: 2px;
                  background-color:shite;
                  color:black;
                  font-weight: bold;
      }
</style>
<body style="text-align: left;">
    <input id="help" type="button" value="Help" onclick="f_help()"/><br><br>
    <table class="calibrate">
      <caption>Set Temp & Humidity Bounds</caption>
        <thead>
            <tr>
                <th>Heat 1 On Temp</th>
                <th>Heat 1 Off Temp</th>
                <th>Humid 1 On %</th>
                <th>Humid 1 Off %</th>
                <th>Heat 2 On Temp</th>
                <th>Heat 2 Off Temp</th>
                <th>Seconds Delay</th>
                <th>
                  <input id="refresh" type="submit" value="Refresh" onclick="f_refresh()" style="margin: 1px; color: black; background-color: yellow; font-weight: bold;">
                  <input id="Submit" type="submit" value="Submit" onclick="f_submit()" style="margin: 2px; color: black; background-color: rgb(248, 138, 171); font-weight: bold;">
                </th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td><input type="number" id="t1_on"  name="limit" min="60" max="87"  onchange="f_updates()"></td>
                <td><input type="number" id="t1_off" name="limit" min="60" max="87"  onchange="f_updates()"></td>
                <td><input type="number" id="h1_on"  name="limit" min="60" max="87"  onchange="f_updates()"></td>
                <td><input type="number" id="h1_off" name="limit" min="60" max="90"  onchange="f_updates()"></td>
                <td><input type="number" id="t2_on"  name="limit" min="60" max="87"  onchange="f_updates()"></td>
                <td><input type="number" id="t2_off" name="limit" min="60" max="87"  onchange="f_updates()"></td>
                <td><input type="number" id="delay"  name="limit" min="2"  max="999" onchange="f_updates()"></td>
                <td></td>
            </tr>
        </tbody>
    </table>
    <p name="settings"  hidden><b>Current MCU Settings (inbound, submitted)</b></p>
    <p name="settings" id="message1" hidden></p>
    <p name="settings" id="message2" hidden>N/A</p>

  </body>
  <script language = "javascript" type = "text/javascript">

      let webSock       = new WebSocket('ws://'+window.location.hostname+':80');
      webSock.onopen    = function(evt){f_webSockOnOpen(evt);}
      webSock.onmessage = function(evt){f_webSockOnMessage(evt);}
      webSock.onerror   = function(evt){f_webSockOnError(evt);}
      webSock.onclose   = function(evt){f_webSockOnClose(evt);}
      function f_webSockOnClose(evt){console.log(evt);}

      let updates = false; // websock updates when false; true after user param change

      function f_webSockOnMessage(evt){
        if(updates)return;
        if(typeof evt.data === "string"){
          j=JSON.parse(evt.data);
          document.getElementById("message1").innerHTML=evt.data;
            for(var key in j[1]){
                if(j[1].hasOwnProperty(key)){
                    if(document.getElementById(key).value=="")
                      document.getElementById(key).value=key!="delay"?j[1][key]:j[1][key]/1000;
                }
            }
        }
      }
      function f_webSockOnOpen(evt){webSock.send("Connect");}
      function f_webSockOnError(evt){}
      function f_submit(){
        var str= '[{"t1":0,"h1":0,"t2":0},{"t1_on":75,"t1_off":82,"h1_on":75,"h1_off":85,"t2_on":75,"t2_off":80,"delay":7000}]';
        j=JSON.parse(str);

        for(var key in j[1]){
          console.log(document.getElementById(key).value);
          j[1][key]=document.getElementById(key).value*1;
          if(key=="delay")j[1][key]=j[1][key]*1000;
        }
        document.getElementById("message2").innerHTML = JSON.stringify(j);
        console.log("sending:", JSON.stringify(j));
        webSock.send(JSON.stringify(j));
        updates = false;  // reset after refresh/submit
      }
      function f_refresh(){
        var x=document.getElementsByName("limit");
        for(var i=0;i<x.length;i++)x[i].value="";
        updates = false;  // reset after refresh/submit
        webSock.send("Connect");
      }
      function f_updates(){
        updates = true;
      }
      function f_help(){
        let v = document.getElementById("help");
        let settings = document.getElementsByName("settings");

        settings.forEach((setting) => {
            setting.hidden = v.value=="Help"?false:true;
          });
          v.value=v.value=="Help"?"Un-Help":"Help";
      }
  </script>
</html>
)=====";

  const char SSIDPWD_HTML[] PROGMEM =  R"=====(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta name="file" content="set_bounds.html">
    <meta name="author" content="Joe Belson 20210822">
    <meta name="what" content="esp8266 html for WiFi connected temp/humidity bounds.">
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Set Temp / Humidity Bounds for 110VAC relays</title>
  </head>


  <body style="text-align: left;">
    <hr>
      <table class="calibrate">
        <caption style="font-weight: bolder;">Set Temp & Humidty Relays SSID : PWD</caption>
          <thead>
              <tr>
                  <th>SSID</th>
                  <th>PWD</th>
              </tr>
          </thead>
          <tbody>
              <tr>
                  <td><input type="text" id="SSID" name="cred" onchange="f_update()"></td>
                  <td><input type="text" id="PWD" name="cred" onchange="f_update()"></td>
                  <td><input type="submit" id="submit" onclick="f_submit()"></td>
              </tr>
          </tbody></table>


      <br>
      <label for="dhcp">DHCP:</label><input type="checkbox" id="isDHCP" onclick="f_dhcp(checked)">
      <br><br>

      <label for="ip"      name="dhcp">IP: </label>     <input type="text"  id="IP"  name="dhcp" style="width: 7rem;" onchange="f_update()">
      <label for="gateway" name="dhcp">Gateway: </label><input type="text" id="GW"   name="dhcp" style="width: 7rem;" onchange="f_update()">
      <label for="mask"    name="dhcp">Mask: </label>   <input type="text" id="MASK" name="dhcp" style="width: 7rem;" onchange="f_update()">
      <br><p id="status"></p>
    </body>
    <script language = "javascript" type = "text/javascript">
  
        let webSock       = new WebSocket('ws://'+window.location.hostname+':80');
        webSock.onopen    = function(evt){webSock.send("Connect");}
        webSock.onmessage = function(evt){f_webSockOnMessage(evt);}
        webSock.onerror   = function(evt){f_webSockOnError(evt);}
        var str= '{"SSID":"0","PWD":"0", "IP":"0","GW":"0","MASK":"0","isDHCP":false}';
        let updates = false;

        function f_webSockOnMessage(evt){
          if(updates)return;
          if(typeof evt.data === "string"){
              document.getElementById("status").innerHTML=evt.data;
              const j=JSON.parse(evt.data);
              const n=document.getElementsByName("dhcp");
              for(var i=0;i<n.length;i++)
                if(n[i].value="")n[i].value=j[n[i].id];
          }
        }
        function f_webSockOnError(evt){}
        function f_submit(){
          j=JSON.parse(str);
          for(var key in j){
            if(key=="isDHCP")j[key]=document.getElementById(key).checked;
            else j[key]=document.getElementById(key).value;
          }
          console.log(j);
          webSock.send(JSON.stringify(j));
          updates = false;
        }
        function f_dhcp(checked){
          const n=document.getElementsByName("dhcp");
          if(checked)for(var i=0;i<n.length;i++)n[i].setAttribute("hidden", "hidden");
          else for(var i=0;i<n.length;i++)n[i].removeAttribute("hidden");
          updates=true;
        }
        function f_update(){
          updates = true;
        }
     </script>
  </html>
      )=====";
