!include nsDialogs.nsh
!include LogicLib.nsh
!include x64.nsh

name "EvtSys - Syslog Collector"
outfile "EvtSys Installer.exe"
RequestExecutionLevel "admin"


Var CDialog
Var CUseDHCP
Var CHost
Var CPort
Var CTag
Var CFacility
Var CLevel

Var UseDHCP
Var Host
Var Port
Var Tag
Var Facility
Var Level

Var Flags


PageEx license
    LicenseData license
PageExEnd
Page directory
Page custom nsConfigureHosts nsConfigureHostsClose
Page custom nsConfigureLogging nsConfigureLoggingClose
Page instfiles
ShowInstDetails show
ShowUninstDetails show

Section "Stop Service"
    ExecWait 'net stop EvtSys'
SectionEnd

Section "Install EvtSys Service"
    SetOutPath $INSTDIR

    ${If} ${RunningX64}
      File Release\x64\evtsys.exe 
    ${Else}
      File Release\x86\evtsys.exe
    ${EndIf}
    File evtsys.cfg

    ExecWait '"$INSTDIR\evtsys.exe" -u'

    WriteUninstaller $INSTDIR\uninstaller.exe
    

    ${If} $UseDHCP == ${BST_CHECKED}
        StrCpy $Flags "-q 1 " 
    ${Else}
        StrCpy $Flags '-h "$Host" -p $Port '
    ${EndIf}

    StrCmp $Tag "" +2
    StrCpy $Flags "$Flags -t $Tag " 
    StrCmp $Tag "" +2
    StrCpy $Flags "$Flags -l $Level "
    StrCmp $Tag "" +2
    StrCpy $Flags "$Flags -f $Facility "   

    ExecWait '"$INSTDIR\evtsys.exe" -i $Flags' $0
    
    ${If} $0 != 0
    ${EndIf}

    ExecWait 'net start EvtSys' $0
SectionEnd

Section "un.Uninstall EvtSys Service" 
    ExecWait 'net stop EvtSys'
    ExecWait '"$INSTDIR\evtsys.exe" -u'
    Delete $INSTDIR\uninstaller.exe
    Delete $INSTDIR\evtsys.exe
SectionEnd

Section "un.Uninstall Complete"
    Delete $INSTDIR\evtsys.cfg
    RMDir $INSTDIR
SectionEnd


Function .onInit
    StrCpy $INSTDIR $programfiles32\evtsys
    ${If} ${RunningX64}
      StrCpy $INSTDIR $programfiles64\evtsys
    ${EndIf}
FunctionEnd

Function nsConfigureHosts

	nsDialogs::Create 1018
	Pop $CDialog

	${If} $CDialog == error
		Abort
	${EndIf}

	${NSD_CreateLabel} 0 0 100% 12u "Configure logging hosts"

	${NSD_CreateLabel} 0 15u 60u 12u "Conf VIA DHCP"
	${NSD_CreateCheckBox} 60u 15u -50u 12u ""
	Pop $CUseDHCP

	${NSD_CreateHLine} 0u 32u 100% 1u ""

	${NSD_CreateLabel} 0 40u 60u 12u "Target Hosts:"
	${NSD_CreateText} 60u 40u -60u 12u ""
	Pop $CHost
	${NSD_CreateLabel} 60u 55u -60u 12u "Separate multiple hosts with `;` "

	${NSD_CreateLabel} 0 70u 60u 12u "Syslog Port:"
	${NSD_CreateText} 60u 70u -60u 12u "514"
	Pop $CPort

	nsDialogs::Show

FunctionEnd

Function nsConfigureHostsClose
    ${NSD_GetState} $CUseDHCP $UseDHCP
    ${NSD_GetText} $CHost $Host
    ${NSD_GetText} $CPort $Port

    
    ${If} $UseDHCP != ${BST_CHECKED}
    ${IF} $Host == ""
    MessageBox MB_OK "A Syslog host is required"
    Abort
    ${EndIf}
    ${EndIf}

FunctionEnd


Function nsConfigureLogging

	nsDialogs::Create 1018
	Pop $CDialog

	${If} $CDialog == error
		Abort
	${EndIf}

	${NSD_CreateLabel} 0 0 100% 12u "Optional Logging Settings"

	${NSD_CreateLabel} 0 15u 60u 12u "Facility"
	${NSD_CreateText} 60u 15u -50u 12u ""
	Pop $CFacility

	${NSD_CreateLabel} 0 30u 60u 12u "Level:"
	${NSD_CreateNumber} 60u 30u -60u 12u ""
	Pop $CLevel
	${NSD_CreateLabel} 60u 45u -60u 12u "0=All/Verbose, 1=Critical, 2=Error, 3=Warning, 4=Info"

	${NSD_CreateLabel} 0 60u 60u 12u "Tag:"
	${NSD_CreateText} 60u 60u -60u 12u ""
	Pop $CTag
	${NSD_CreateLabel} 60u 75u -60u 12u "Include tag as program field in syslog message"

	nsDialogs::Show

FunctionEnd

Function nsConfigureLoggingClose
    ${NSD_GetText} $CTag $Tag
    ${NSD_GetText} $CLevel $Level
    ${NSD_GetText} $CFacility $Facility
FunctionEnd