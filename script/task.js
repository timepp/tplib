function InitTasks()
{

G.path_greensoft = "e:\\greensoft";

//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "关闭系统还原";
// TODO 寻找检查和关闭系统还原的有效方法
op.check = function ()
{
    var rs = WMI("default").InstancesOf("SystemRestore");
    return rs.Count == 0;
}
op.run = function()
{
    var sr = WMI("default").Get("SystemRestore");
    var func = sr.Methods_.item("Disable");
    var param = func.InParameters.SpawnInstance_();
    param.Drive = "C:\\";
    var ret = sr.ExecMethod_(func.Name, param);
}

//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "安装daemon tools";
op.check = function()
{
    return RegIsStringValueExist(HKEY_LOCAL_MACHINE, "SOFTWARE\\DT Soft\\DAEMON Tools Pro\\Data", "HP");
};



//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "安装VS2005, sp1";
op.check = function()
{
    return RegIsStringValueExist(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\VisualStudio\\8.0", "InstallDir");
};


//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "把autoit放入启动组";
op.check = function()
{
    if (Glob(fso.GetFolder(G.path_autorun), /autoit/i).length > 0) return true;
    if (Glob(fso.GetFolder(G.path_autorun_alluser), /autoit/i).length > 0) return true;
    return false;
}
op.run = function()
{
    var dir = G.path_greensoft + "\\psoft\\AutoIt";
    CreateLnk(G.path_autorun_alluser + "\\AutoIt.lnk", dir + "\\AutoIt3.exe", dir, "\"hotkey.au3\"");
    return true;
}


//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "把handyrun放入启动组";
op.check = function()
{
    if (Glob(fso.GetFolder(G.path_autorun), /handyrun/i).length > 0) return true;
    if (Glob(fso.GetFolder(G.path_autorun_alluser), /handyrun/i).length > 0) return true;
    return false;
}
op.run = function()
{
    var dir = G.path_greensoft + "\\HandyRun";
    CreateLnk(G.path_autorun_alluser + "\\HandyRun.lnk", dir + "\\HandyRun.exe", dir);
    return true;
}


//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "安装字体megatops procoder, anonymous";
op.fonts = new Array("Anonymous.ttf", "MegatopsProCoder1.0.fon");
op.check = function()
{
    var shell_dir = shellapp.namespace(SHN_FONTS);
    var dir = shell_dir.Self.Path;
    for (var i = 0; i < this.fonts.length; i++)
    {
        if (!fso.FileExists(dir + "\\" + this.fonts[i])) return false;
    }
    return true;
}
op.run = function() 
{
    var font_dir = shellapp.namespace(SHN_FONTS);
    for (var i = 0; i < this.fonts.length; i++)
    {
        font_dir.CopyHere(G.path_conf + "\\字体\\" + this.fonts[i]);
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "安装acrobat";
op.check = function()
{
    return RegIsStringValueExist(HKEY_LOCAL_MACHINE, "SOFTWARE\\Adobe\\Adobe Acrobat\\9.0\\Installer", "Path");
}


//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "安装live mesh";
op.check = function()
{
    return RegIsStringValueExist(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{DCB4E1D9-B187-4B54-971E-1478485C9A53}", "DisplayName");
}

//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "设置路径";
op.pathToAdd = "%GS%\\svn\\bin;%GS%\\cvsnt;%GS%\\psoft\\cmdline;".replace(/%GS%/g, G.path_greensoft);
op.check = function()
{
    return IsInPathEnv(this.pathToAdd);
};
op.run = function()
{
    return ChangePathEnv(this.pathToAdd, "");
};


//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "设置BAIDUHI_THIRDSRC环境变量";
op.check = function() 
{
    var val = GetEnv("BAIDUHI_THIRDSRC");
    return (val != null);
}
op.run = function()
{
    SetEnv("BAIDUHI_THIRDSRC", "d:\\IMSource\\app\\gensoft\\dialog\\thirdsrc");
}




//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "安装输入法";
op.check = function()
{
    // 海峰或其它?
    if (fso.GetFolder("C:\\Program Files\\freeime")) return true;
    
    return false;
}

//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "安装office";
op.check = function()
{
    return RegIsStringValueExist(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Office\\14.0\\Common\\InstallRoot", "Path");
}

//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "设置始终显示托盘图标";
op.check = function()
{
    var val = RegGetDWORDValue(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", "EnableAutoTray");
    return val == 0;
}
op.run = function()
{
    RegSetDWORDValue(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", "EnableAutoTray", 0);
    alert("操作已成功，但是不能即时反映出效果。需要重启explorer.exe才能生效。");
    return true;
}

//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "激活windows";


//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "安装flash player";
op.check = function()
{
    return RegIsStringValueExist(HKEY_LOCAL_MACHINE, "SOFTWARE\\Macromedia\\FlashPlayerActiveX", "Path");
}

//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "在资源管理器里显示文件扩展名";
op.check = function()
{
    var val = RegGetDWORDValue(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "HideFileExt");
    return val == 0;
}
op.run = function()
{
    RegSetDWORDValue(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "HideFileExt", 0);
    return true;
}

//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "normal.dotm中加入常用style";


//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "导入CVS配置";


//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "导入VC配置";


//////////////////////////////////////////////////////////////////////////
var op = new Object;
ops.push(op);
op.name = "把autoexp.dat拷到VC的目录下";
op.src = "";
op.dest = "";
op.init = function()
{
    this.src = G.path_conf + "\\autoexp.dat";
    var dir = RegGetStringValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\VisualStudio\\8.0", "InstallDir");
    if (dir)
    {
        var autoexp_dir = dir.replace(/^(.*)\\IDE\\?$/i, "$1\\Packages\\Debugger\\");
        this.dest = autoexp_dir + "autoexp.dat";
    }
}
op.check = function()
{
    return !IsNeedCopy(this.src, this.dest);
}
op.run = function()
{
    CopyFile(this.src, this.dest);
    return true;
}

} // end function InitTasks
