function doGet(e) {
  return ContentService.createTextOutput("Web app is running. Use POST.");
}

function doPost(e) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();

  const p = (e && e.parameter) ? e.parameter : {};
  const device = p.device || "esp32";
  const event  = p.event  || "TEST";
  const note   = p.note   || "";

  sheet.appendRow([new Date(), device, event, note]);

  return ContentService.createTextOutput("OK");
}
