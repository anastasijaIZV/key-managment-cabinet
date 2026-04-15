function doGet(e) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var usersSheet = ss.getSheetByName("Users");
  var keysSheet = ss.getSheetByName("Keys");
  var logsSheet = ss.getSheetByName("Logs");

  var mode = (e.parameter.mode || "").toString().trim().toLowerCase();

  if (mode === "classify") {
    var uid = normalizeUid(e.parameter.uid);
    return ContentService.createTextOutput(classifyUid(usersSheet, keysSheet, uid));
  }

  if (mode === "process") {
    var action = (e.parameter.action || "").toString().trim().toUpperCase();
    var userUid = normalizeUid(e.parameter.useruid);
    var keyUid = normalizeUid(e.parameter.keyuid);
    return ContentService.createTextOutput(processAction(usersSheet, keysSheet, logsSheet, action, userUid, keyUid));
  }

  return ContentService.createTextOutput("INVALID_MODE");
}

function normalizeUid(value) {
  return (value || "")
    .toString()
    .trim()
    .toUpperCase()
    .replace(/[^0-9A-F]/g, "");
}

function classifyUid(usersSheet, keysSheet, uid) {
  if (!uid) return "MISSING_UID";

  var userResult = findUser(usersSheet, uid);
  var keyResult = findKey(keysSheet, uid);

  if (userResult.found && keyResult.found) {
    return "DUPLICATE_UID";
  }

  if (userResult.found) {
    if (!userResult.active) return "USER_INACTIVE";
    return "USER_FOUND|" + userResult.uid + "|" + userResult.name;
  }

  if (keyResult.found) {
    return "KEY_FOUND|" + keyResult.uid + "|" + keyResult.name + "|" + keyResult.status + "|" + keyResult.holderUid;
  }

  return "NOT_FOUND";
}

function processAction(usersSheet, keysSheet, logsSheet, action, userUid, keyUid) {
  if (!action || !userUid || !keyUid) {
    return "MISSING_PARAMS";
  }

  var userResult = findUser(usersSheet, userUid);
  if (!userResult.found) return "USER_NOT_FOUND";
  if (!userResult.active) return "USER_INACTIVE";

  var keyResult = findKey(keysSheet, keyUid);
  if (!keyResult.found) return "KEY_NOT_FOUND";

  if (action === "TAKE") {
    if (String(keyResult.status).toUpperCase() !== "IN") {
      return "KEY_NOT_AVAILABLE";
    }

    keysSheet.getRange(keyResult.row, 3).setValue("OUT");
    keysSheet.getRange(keyResult.row, 4).setValue(userResult.uid);

    logsSheet.appendRow([
      new Date(),
      "TAKE",
      userResult.uid,
      userResult.name,
      keyResult.uid,
      keyResult.name,
      "OK"
    ]);

    return "OK|TAKE|" + userResult.name + "|" + keyResult.name;
  }

  if (action === "RETURN") {
    if (String(keyResult.status).toUpperCase() !== "OUT") {
      return "KEY_ALREADY_IN";
    }

    keysSheet.getRange(keyResult.row, 3).setValue("IN");
    keysSheet.getRange(keyResult.row, 4).setValue("");

    logsSheet.appendRow([
      new Date(),
      "RETURN",
      userResult.uid,
      userResult.name,
      keyResult.uid,
      keyResult.name,
      "OK"
    ]);

    return "OK|RETURN|" + userResult.name + "|" + keyResult.name;
  }

  return "INVALID_ACTION";
}

function findUser(sheet, uid) {
  var lastRow = sheet.getLastRow();
  if (lastRow < 2) {
    return { found: false };
  }

  var values = sheet.getRange(2, 1, lastRow - 1, 3).getValues();

  for (var i = 0; i < values.length; i++) {
    var sheetUid = normalizeUid(values[i][0]);
    var name = (values[i][1] || "").toString().trim();
    var active = values[i][2] === true || String(values[i][2]).toUpperCase() === "TRUE";

    if (sheetUid === uid) {
      return {
        found: true,
        row: i + 2,
        uid: sheetUid,
        name: name,
        active: active
      };
    }
  }

  return { found: false };
}

function findKey(sheet, uid) {
  var lastRow = sheet.getLastRow();
  if (lastRow < 2) {
    return { found: false };
  }

  var values = sheet.getRange(2, 1, lastRow - 1, 4).getValues();

  for (var i = 0; i < values.length; i++) {
    var sheetUid = normalizeUid(values[i][0]);
    var name = (values[i][1] || "").toString().trim();
    var status = (values[i][2] || "").toString().trim().toUpperCase();
    var holderUid = normalizeUid(values[i][3]);

    if (sheetUid === uid) {
      return {
        found: true,
        row: i + 2,
        uid: sheetUid,
        name: name,
        status: status,
        holderUid: holderUid
      };
    }
  }

  return { found: false };
}
