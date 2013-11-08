
function indexForIconName(iconName) {
  switch (iconName) {
    case "01d":
      return "B";
    case "01n":
      return "C";
    case "02d":
      return "H";
    case "02n":
      return "I";
    case "03d":
    case "03n":
      return "N";
    case "04d":
    case "04n":
      return "Y";
    case "09d":
    case "09n":
    case "10d":
    case "10n":
      return "R";
    case "11d":
    case "11n":
      return "Z";
    case "13d":
    case "13n":
      return "W";
    case "50d":
      return "J";
    case "50n":
      return "E";
    default:
      return "";
  }
}

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();
  req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        //console.log(req.responseText);
        response = JSON.parse(req.responseText);
        var temperature, icon;
        if (response) {
          temperature = Math.round((response.main.temp - 273.15) * 1.8 + 32);
          icon = indexForIconName(response.weather[0].icon);
          console.log("temp " + temperature);
          console.log("icon " + icon);
          Pebble.sendAppMessage({
            "icon":icon,
            "temperature":temperature + "\u00B0F",
          });
        }
      } else {
        console.log("Error");
      }
    }
  }
  req.send(null);
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
  fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    "icon":11,
    "temperature":""
  });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };

Pebble.addEventListener("ready",
                        function(e) {
                          console.log("connect!" + e.ready);
                          window.navigator.geolocation.getCurrentPosition(locationSuccess,
                                                                          locationError,
                                                                          locationOptions);
                          console.log(e.type);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                          window.navigator.geolocation.getCurrentPosition(locationSuccess,
                                                                          locationError,
                                                                          locationOptions);
                          console.log(e.type);
                          console.log(e.payload.temperature);
                          console.log("message!");
                        });
