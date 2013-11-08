
function iconFromWeatherId(iconName) {
  switch (iconName) {
    case "01d":
      return 0;
    case "01n":
      return 1;
    case "02d":
      return 2;
    case "02n":
      return 3;
    case "03d":
    case "03n":
      return 4;
    case "04d":
    case "04n":
      return 5;
    case "09d":
    case "09n":
    case "10d":
    case "10n":
      return 6;
    case "11d":
    case "11n":
      return 7;
    case "13d":
    case "13n":
      return 8;
    case "50d":
      return 9;
    case "50n":
      return 10;
    default:
      return 11;
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
        console.log(req.responseText);
        response = JSON.parse(req.responseText);
        var temperature, icon, city;
        if (response) {
          temperature = Math.round((response.main.temp - 273.15) * 1.8 + 32);
          icon = iconFromWeatherId(response.weather[0].icon);
          city = response.name;
          console.log("temp " + temperature);
          console.log("icon " + icon);
          console.log("city " + city);
          Pebble.sendAppMessage({
            "icon":icon,
            "temperature":temperature + "\u00B0F",
            "city":city});
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
    "icon":7,
    "temperature":"N/A",
    "city":"Loc Unavailable"
  });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };

Pebble.addEventListener("ready",
                        function(e) {
                          console.log("connect!" + e.ready);
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                          console.log(e.payload.temperature);
                          console.log("message!");
                        });
