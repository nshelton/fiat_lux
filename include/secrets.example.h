// copy to include/secrets.h and edit -- secrets.h is gitignored
#pragma once

static const char sec_ssid[] = "your-ssid";
static const char sec_pass[] = "your-password";

// api.open-meteo.com request path -- no API key, just set your lat/lon
static const char weather_api_path[] =
    "/v1/forecast?latitude=34.00&longitude=-118.40"
    "&current=temperature_2m,relative_humidity_2m&temperature_unit=fahrenheit"
    "&hourly=temperature_2m,relative_humidity_2m"
    "&daily=sunrise,sunset&forecast_days=1&timezone=auto";

// air-quality-api.open-meteo.com request path -- same lat/lon
static const char aqi_api_path[] =
    "/v1/air-quality?latitude=34.00&longitude=-118.40"
    "&current=us_aqi&hourly=us_aqi&forecast_days=1&timezone=auto";

// must match --auth in platformio.ini [env:ota]
static const char ota_password[] = "fiatlux";
