function doGet(e) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();

  var usersSheet = ss.getSheetByName("Users");
  var keysSheet = ss.getSheetByName("Keys");
  var logsSheet = ss.getSheetByName("Logs");

  if (!usersSheet || !keysSheet || !logsSheet) {
    return ContentService.createTextOutput("ERROR: Missing required sheet");
  }

  var action = (e.parameter.action || "").toUpperCase().trim();
  var userUID = normalizeUID(e.parameter.userUID || "");
  var keyUID = normalizeUID(e.parameter.keyUID || "");

  if (!action || !userUID || !keyUID) {
    return ContentService.createTextOutput("ERROR: Missing action, userUID, or keyUID");
  }

  if (action !== "TAKE" && action !== "RETURN") {
    return ContentService.createTextOutput("ERROR: Invalid action");
  }

  var user = findUser(usersSheet, userUID);
  if (!user.found) {
    logEvent(logsSheet, action, userUID, "", keyUID, "", "ERROR", "Unknown user");
    return ContentService.createTextOutput("ERROR: Unknown user");
  }

  if (!user.active) {
    logEvent(logsSheet, action, userUID, user.name, keyUID, "", "ERROR", "Inactive user");
    return ContentService.createTextOutput("ERROR: Inactive user");
  }

  var key = findKey(keysSheet, keyUID);
  if (!key.found) {
    logEvent(logsSheet, action, userUID, user.name, keyUID, "", "ERROR", "Unknown key");
    return ContentService.createTextOutput("ERROR: Unknown key");
  }

  if (action === "TAKE") {
    if (key.status === "OUT") {
      logEvent(logsSheet, action, userUID, user.name, keyUID, key.name, "ERROR", "Key already OUT");
      return ContentService.createTextOutput("ERROR: Key already OUT");
    }

    keysSheet.getRange(key.row, 3).setValue("OUT");      // Status
    keysSheet.getRange(key.row, 4).setValue(userUID);    // HolderUID

    logEvent(logsSheet, action, userUID, user.name, keyUID, key.name, "OK", "");
    return ContentService.createTextOutput("OK: TAKE logged");
  }

  if (action === "RETURN") {
    if (key.status === "IN") {
      logEvent(logsSheet, action, userUID, user.name, keyUID, key.name, "ERROR", "Key already IN");
      return ContentService.createTextOutput("ERROR: Key already IN");
    }

    keysSheet.getRange(key.row, 3).setValue("IN");       // Status
    keysSheet.getRange(key.row, 4).setValue("");         // HolderUID

    logEvent(logsSheet, action, userUID, user.name, keyUID, key.name, "OK", "");
    return ContentService.createTextOutput("OK: RETURN logged");
  }

  return ContentService.createTextOutput("ERROR: Unexpected");
}

function normalizeUID(uid) {
  return String(uid).trim().toUpperCase();
}

function findUser(sheet, userUID) {
  var data = sheet.getDataRange().getValues();

  for (var i = 1; i < data.length; i++) {
    var uid = normalizeUID(data[i][0]);
    var name = data[i][1];
    var activeValue = String(data[i][2]).toUpperCase().trim();

    var active = (activeValue === "TRUE" || activeValue === "YES" || activeValue === "1");

    if (uid === userUID) {
      return {
        found: true,
        row: i + 1,
        uid: uid,
        name: name,
        active: active
      };
    }
  }

  return { found: false };
}

function findKey(sheet, keyUID) {
  var data = sheet.getDataRange().getValues();

  for (var i = 1; i < data.length; i++) {
    var uid = normalizeUID(data[i][0]);
    var name = data[i][1];
    var status = String(data[i][2]).toUpperCase().trim();
    var holderUID = normalizeUID(data[i][3] || "");

    if (uid === keyUID) {
      return {
        found: true,
        row: i + 1,
        uid: uid,
        name: name,
        status: status,
        holderUID: holderUID
      };
    }
  }

  return { found: false };
}

function logEvent(logsSheet, action, userUID, userName, keyUID, keyName, result, notes) {
  logsSheet.appendRow([
    new Date(),
    action,
    userUID,
    userName,
    keyUID,
    keyName,
    result,
    notes
  ]);
}
