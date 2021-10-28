String getIndexPage(
  bool pilotStatus,
  bool heatStatus,
  int pilotVoltage) {

  String s = "<html>";

  s += "<head>";
  s += "<meta http-equiv='refresh' content='2'>";
  s += "</head>";

  // ----------------------------------------

  s += "<body>";

  s += "<h1>Fireplace Usage Counter</h1>";
  s += "<hr />";

  // ----------------------------------------

  s += "Pilot light is ";

  if (pilotStatus)
    s += "<span style='color: green; font-weight: bold;'>ON</span>";
  else
    s += "<span style='color: red; font-weight: bold;'>OFF</span>";

  s += " (" + String(pilotVoltage * 0.0008056640625) + "v)";

  s += "<br />";

  // ----------------------------------------

  s += "Heat is ";

  if (heatStatus)
    s += "<span style='color: green; font-weight: bold;'>ON</span>";
  else
    s += "<span style='color: red; font-weight: bold;'>OFF</span>";

  s += "<br />";

  if (!pilotStatus) {

    s += "<br />";
    s += "<h2>Diagnostic</h2>";
    s += "<p>";
    s += "It appears the pilot light is out. The pilot light is either manually turned off or the propane tank is empty.";
    s += "</p>";
  }

  // ----------------------------------------

  s += "</body>";
  s += "</html>";

  return s;
}
