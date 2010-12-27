document.write("<script src='json2.js' type='text/javascript'></script>");

//常量
var ForReading = 1, ForWriting = 2;
var HKEY_LOCAL_MACHINE = 0x80000002;
var HKEY_CURRENT_USER = 0x80000001;
var HKEY_CLASSES_ROOT = 0x80000000;
var SHN_FONTS = 0x14;

// 对象
var fso = new ActiveXObject("Scripting.FileSystemObject");
var shell = new ActiveXObject("WScript.Shell");
var shellapp = new ActiveXObject("Shell.Application");

// 其他
var g_wmi = null;

function WMI(path) {
    if (!g_wmi) {
        g_wmi = new Object;
    }
    if (!g_wmi[path]) {
        g_wmi[path] = GetObject("winmgmts:root/" + path);
    }
    return g_wmi[path];
}

function REG() {
    return WMI("default").Get("StdRegProv");
}

function IsAdmin() {
    var oNet = new ActiveXObject("WScript.Network");
    var oGroup = GetObject("WinNT://./Administrators");
    var e = new Enumerator(oGroup.Members());
    for (; !e.atEnd(); e.moveNext()) {
        if (e.item().Name == oNet.UserName) return true;
    }
    return false;
}

function HasFullPrivilege() {
    try {
        var value = shell.RegRead("HKEY_USERS\\S-1-5-19\\");
    }
    catch (e) {
        return false;
    }
    return true;
}

function ElevatePrivilege(cmdline) {
    if (!HasFullPrivilege()) {
        var oNet = new ActiveXObject("WScript.Network");
        var ret = shellapp.ShellExecute("mshta.exe", cmdline + "--uac --user=" + oNet.UserName, "", "runas", 1);
        window.close();
        return true;
    }
    return false;
}

function InvokeCommonRegTask(cmd, root, key, valname, val) {
    var func = REG().Methods_.Item(cmd);
    var param = func.InParameters.SpawnInstance_();
    param.hDefKey = root;
    param.sSubKeyName = key;
    param.sValueName = valname;
    if (val != null) {
        if (typeof (val) == "string") param.sValue = val;
        else param.uValue = val;
    }
    return REG().ExecMethod_(func.Name, param);
}
function RegGetStringValue(root, key, val) {
    var ret = InvokeCommonRegTask("GetStringValue", root, key, val);
    return ret.sValue;
}
function RegIsStringValueExist(root, key, val) {
    var s = RegGetStringValue(root, key, val);
    return s != undefined && s != null;
}
function RegGetDWORDValue(root, key, val) {
    return InvokeCommonRegTask("GetDWORDValue", root, key, val).uValue;
}
function RegSetDWORDValue(root, key, valname, val) {
    return InvokeCommonRegTask("SetDWORDValue", root, key, valname, val);
}

function ResizeWindow(cx, cy, center) {
    window.resizeTo(cx, cy);
    if (center) {
        var items = WMI("cimv2").ExecQuery("Select * From Win32_DesktopMonitor");
        var item = new Enumerator(items).item();
        var w = item.ScreenWidth;
        var h = item.ScreenHeight;
        window.moveTo((w - cx) / 2, (h - cy) / 2);
    }
}

function ClearTable(tbl) {
    while (tbl.rows.length > 0) {
        tbl.deleteRow(0);
    }
}

function AddGradientBK(o, c1, c2, tp) {
    if (c2 == null) c2 = "#FFFFFF";
    if (tp == null) tp = 1;
    o.style.filter = "progid:DXImageTransform.Microsoft.Gradient(GradientType=%tp,StartColorStr='%sc', EndColorStr='%ec')".replace("%sc", c1).replace("%ec", c2).replace("%tp", tp);
}

function CenterAbsoluteObject(o) {
    o.style.left = (document.body.clientWidth - o.offsetWidth) / 2;
    o.style.top = (document.body.clientHeight - o.offsetHeight) / 2;
}

function PopupMessage(title, msg) {
    var oDiv = document.createElement("div");
    oDiv.id = "div_err";
    oDiv.style.cssText = "padding: 5px; border: medium #0000FF double; position: absolute; font-family: monospace; background-color:#FFFFFF";

    var oSpan = document.createElement("span");
    oSpan.innerHTML = title + "<br/>";
    oSpan.style.fontWeight = "bold";
    oDiv.appendChild(oSpan);

    oSpan = document.createElement("span");
    oSpan.innerText = msg;
    oSpan.style.fontSize = "small";
    oDiv.appendChild(oSpan);

    oSpan = document.createElement("div");
    oSpan.align = "center";
    oSpan.innerHTML = "<button onclick=\"document.body.removeChild(document.getElementById('div_err'))\">关闭</button>";
    oDiv.appendChild(oSpan);
    document.body.appendChild(oDiv);
    CenterAbsoluteObject(oDiv);
}

function Glob(dir, fn_regex) {
    var r = new Array;
    var e = new Enumerator(dir.files);
    for (; !e.atEnd(); e.moveNext()) {
        var f = e.item();
        if (f.Path.search(fn_regex) != -1) {
            r.push(f.Path);
        }
    }
    return r;
}

function CreateLnk(lnkName, targetPath, dir, argument, desc) {
    var oLnk = shell.CreateShortcut(lnkName);
    oLnk.TargetPath = targetPath;
    oLnk.WorkingDirectory = dir;
    oLnk.Description = desc
    oLnk.Arguments = argument;
    oLnk.Save();
}

function IndexOf(arr, str, nocase) {
    for (var i = 0; i < arr.length; i++) {
        if (nocase) {
            if (arr[i].toLowerCase() == str.toLowerCase()) return i;
        }
        else {
            if (arr[i] == str) return i;
        }
    }
    return -1;
}

function GetEnv(vname) {
    var items = WMI("cimv2").ExecQuery("Select * from Win32_Environment Where Name = '$V'".replace("$V", vname));
    if (!items || items.Count == 0) return null;
    var item = new Enumerator(items).item();
    Log("$K --> $V".replace("$K", vname).replace("$V", item.VariableValue));
    return item.VariableValue;
}

function SetEnv(vname, val, login_name) {
    if (login_name == null) login_name = "<SYSTEM>";

    var item = WMI("cimv2").Get("Win32_Environment").SpawnInstance_();
    item.Name = vname;
    item.Username = login_name;
    item.VariableValue = val;
    item.Put_();
}

function IsInPathEnv(path) {
    var items = WMI("cimv2").ExecQuery("Select * from Win32_Environment Where Name = 'PATH'");
    var item = new Enumerator(items).item();
    var val = item.VariableValue.toLowerCase() + ";";

    var ps = path.split(";");
    for (var i = 0; i < ps.length; i++) {
        if (ps[i] != "" && val.indexOf(ps[i].toLowerCase() + ";") == -1) return false;
    }
    return true;
}
function ChangePathEnv(pathToAdd, pathToDel) {
    var items = WMI("cimv2").ExecQuery("Select * from Win32_Environment Where Name = 'PATH'");
    var item = new Enumerator(items).item();

    Log("旧路径：" + item.VariableValue);
    Log("需要增加：" + pathToAdd);
    Log("需要删除：" + pathToDel)

    var val = item.VariableValue;
    var ps = val.split(";");
    var ps_add = pathToAdd.split(";");
    var ps_del = pathToDel.split(";");
    var ps_final = new Array;

    ps = ps.concat(ps_add);
    for (var i = 0; i < ps.length; i++) {
        if (ps[i] == "") continue;
        if (IndexOf(ps_del, ps[i], true) >= 0) continue;
        if (IndexOf(ps_final, ps[i], true) >= 0) continue;
        ps_final.push(ps[i]);
    }

    item.VariableValue = ps_final.join(";");
    item.Put_();
    Log("新路径：" + item.VariableValue);
    return true;
}

function IsNeedCopy(src, dest) {
    if (!fso.FileExists(src)) {
        throw "源[" + src + "]不存在";
    }
    if (!dest) {
        throw "目标为空";
    }
    if (!fso.FileExists(dest)) return true;
    var f1 = fso.GetFile(src);
    var f2 = fso.GetFile(dest);
    if (f1 && f2 && f1.Size == f2.Size) return false;
    return true;
}

function CopyFile(src, dest) {
    if (!fso.FileExists(src)) {
        throw "源[" + src + "]不存在";
    }
    if (!dest) {
        throw "目标为空";
    }

    var f1 = fso.GetFile(src);
    f1.Copy(dest);
    Log("[$1] --> [$2]".replace("$1", src).replace("$2", dest));
    return true;
}

function RunCommand(cmd, a, b) {
    shell.Run('"' + cmd + '"', a, b);
}
function GetErrDesc(e) {
    if (e instanceof Error) return e.description;
    return e;
}
function GetCurTime() {
    var ct = new Date();
    var D2 = function (n) { if (n < 10) return "0" + n.toString(); return n.toString(); };
    return D2(ct.getHours()) + ":" + D2(ct.getMinutes()) + ":" + D2(ct.getSeconds());
}
function SetText(obj, text, color) {
    if (color == undefined) color = "black";
    obj.innerHTML = text.fontcolor(color);
}
function SplitCommandLine(cmdline) {
    args = new Array;
    var re = /^\s*("[^"]+"|[^" ]+)/;
    while ((arr = re.exec(cmdline)) != null) {
        args.push(arr[1].replace(/^("?)(.*)\1$/, "$2"));
        cmdline = cmdline.substr(arr.lastIndex);
    }
    return args;
}
// 取文本文件信息, \r\n会被替换为\n， 文件中不能有控制字符
function GetTextFileContent(filename) {
    var content = "";
    try {
        var ofile = fso.OpenTextFile(filename, ForReading);
        content = ofile.ReadAll();
        ofile.Close();
    }
    catch (e) {
        alert(e.message);
    }
    return content;
}

function SaveFile(filename, text) {
	try {
		var ofile = fso.OpenTextFile(filename, ForWriting, true);
		ofile.Write(text);
		ofile.Close();
	}
	catch (e) {
		alert(e.message);
	}
}

function PlainTextToHTML(text) {
    var html = "";
    for (var i = 0; i < text.length; i++) {
        var code = text.charCodeAt(i);
        switch (code) {
            case 38: html += "&amp;"; break;
            case 60: html += "&lt;"; break;
            case 62: html += "&gt;"; break;
            case 10: html += "<br />"; break;
            default: html += String.fromCharCode(code);
        }
    }
    return html;
}

// 取二进制文件信息，文件内容按原貌读取
function GetBinaryFileContent(filename, pos, len) {
    var stream = new ActiveXObject("ADODB.Stream");
    //	stream.Type = 1;
    stream.Open();
    stream.LoadFromFile(filename);
    stream.Position = pos;
    var str = "";
    //	str = stream.Read(len);
    str = stream.ReadText(len);
    return DumpBin(str);
}

function DumpBin(str, npl) {
    var codeMap = "0123456789ABCDEF";
    function hex(str, pos) {
        var n = str.charCodeAt(pos);
        return codeMap[n / 16] + codeMap[n % 16];
    }

    var outstr;
    var i, j, pos;
    for (i = 0; i < str.length; i += npl) {
        for (j = 0; j < npl; j++) {
            pos = i * npl + j;
            if (pos >= str.length) {
                outstr += "   ";
            }
            else {
                outstr += hex(str, pos) + " ";
            }
        }

        outstr += " ";
        for (j = 0; j < npl; j++) {
            pos = i * npl + j;
            if (pos > str.length) {
                outstr += " ";
            }
            else {
                var n = str.charCodeAt(pos);
                outstr += (n >= 0x20 && n < 0x80) ? String.fromCharCode(n) : ".";
            }
        }

        outstr += "\n";
    }
}

function SaveObject(obj, filename) {
    var f = fso.OpenTextFile(filename, ForWriting, true, -1);
    f.Write(JSON.stringify(obj));
    f.Close();
}

function LoadObject(filename) {
    try {
        var f = fso.OpenTextFile(filename, ForReading, false, -1);
        if (f) {
            return JSON.parse(f.ReadAll());
        }
    }
    catch (e) {
    }

    return null;
}
