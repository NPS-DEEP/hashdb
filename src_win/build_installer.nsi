# NSIS script for creating the Windows hashdb installer file.
#
# Installs the following:
#   64-bit configuration of hashdb tools
#   pdf documents
#   path
#   uninstaller
#   start menu shortcut for pdf documents
#   uninstaller shurtcut
#   registry information including uninstaller information

# Assign VERSION externally with -DVERSION=<ver>
# Build from signed files with -DSIGN
!ifndef VERSION
	!echo "VERSION is required."
	!echo "example usage: makensis -DVERSION=1.0.0 build_installer.nsi"
	!error "Invalid usage"
!endif

!define APPNAME "HashDB ${VERSION}"
!define REG_SUB_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
!define COMPANYNAME "Naval Postgraduate School"
!define DESCRIPTION "Hash Database Tools"

# These will be displayed by the "Click here for support information" link in "Add/Remove Programs"
# It is possible to use "mailto:" links here to open the email client
!define HELPURL "//https://github.com/simsong/hashdb" # "Support Information" link
!define UPDATEURL "//https://github.com/simsong/hashdb" # "Product Updates" link
!define ABOUTURL "//https://github.com/simsong/hashdb" # "Publisher" link

SetCompressor lzma
 
RequestExecutionLevel admin
 
InstallDir "$PROGRAMFILES64\${APPNAME}"
 
Name "${APPNAME}"
	outFile "hashdb-${VERSION}-windowsinstaller.exe"
 
!include LogicLib.nsh
!include EnvVarUpdate.nsi
!include x64.nsh
 
page components
Page instfiles
UninstPage instfiles
 
!macro VerifyUserIsAdmin
UserInfo::GetAccountType
pop $0
${If} $0 != "admin" ;Require admin rights on NT4+
	messageBox mb_iconstop "Administrator rights required!"
	setErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
	quit
${EndIf}
!macroend

#Section "hashdb"
Section "${APPNAME}"
        # establish out path
        setOutPath "$INSTDIR"

	# install Registry information
	WriteRegStr HKLM "${REG_SUB_KEY}" "DisplayName" "${APPNAME} - ${DESCRIPTION}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "${REG_SUB_KEY}" "QuietUninstallString" "$INSTDIR\uninstall.exe /S"
	WriteRegStr HKLM "${REG_SUB_KEY}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "${REG_SUB_KEY}" "Publisher" "${COMPANYNAME}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "HelpLink" "${HELPURL}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "URLUpdateInfo" "${UPDATEURL}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "URLInfoAbout" "${ABOUTURL}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "DisplayVersion" "${VERSION}"
	# There is no option for modifying or repairing the install
	WriteRegDWORD HKLM "${REG_SUB_KEY}" "NoModify" 1
	WriteRegDWORD HKLM "${REG_SUB_KEY}" "NoRepair" 1

	# install the uninstaller
	# create the uninstaller
	writeUninstaller "$INSTDIR\uninstall.exe"
 
	# create the start menu for hashdb
	createDirectory "$SMPROGRAMS\${APPNAME}"

	# link the uninstaller to the start menu
	createShortCut "$SMPROGRAMS\${APPNAME}\Uninstall ${APPNAME}.lnk" "$INSTDIR\uninstall.exe"

	# install hashdb
        file "hashdb.exe"

        # install PDF doc
        setOutPath "$INSTDIR\pdf"
        file "hashdbUsersManual.pdf"

        # install shortcut to PDF doc
	createShortCut "$SMPROGRAMS\${APPNAME}\hashdb Users Manual.lnk" "$INSTDIR\pdf\hashdbUsersManual.pdf"
sectionEnd

Section "Add to path"
	setOutPath "$INSTDIR"
        # add hashdb to system PATH
        ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR"
sectionEnd

function .onInit
        # only win64 is supported
        ${If} ${RunningX64}
            # good
        ${Else}
	    MessageBox MB_ICONSTOP \
		"Error: ${APPNAME} is not supported on 32-bit Windows systems.  Please use Win-64."
	    Abort
        ${EndIf}

        # require admin
	setShellVarContext all
	!insertmacro VerifyUserIsAdmin
functionEnd

function un.onInit
	SetShellVarContext all
 
	#Verify the uninstaller - last chance to back out
	MessageBox MB_OKCANCEL "Permanantly remove ${APPNAME}?" IDOK next
		Abort
	next:
	!insertmacro VerifyUserIsAdmin
functionEnd
 
Function un.FailableDelete
	Start:
	delete "$0"
	IfFileExists "$0" FileStillPresent Continue

	FileStillPresent:
	DetailPrint "Unable to delete file $0, likely because it is in use.  Please close all hashdb files and try again."
	MessageBox MB_ICONQUESTION|MB_RETRYCANCEL \
		"Unable to delete file $0, \
		likely because it is in use.  \
		Please close all Bulk Extractor files and try again." \
 		/SD IDABORT IDRETRY Start IDABORT InstDirAbort

	# abort
	InstDirAbort:
	DetailPrint "Uninstall started but did not complete."
	Abort

	# continue
	Continue:
FunctionEnd

section "uninstall"
	# manage uninstalling these because they may be open
	StrCpy $0 "$INSTDIR\hashdb.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\pdf\hashdbUsersManual.pdf"
	Call un.FailableDelete

#	# uninstall files and links
#	delete "$INSTDIR\32-bit\*"
#	delete "$INSTDIR\64-bit\*"
#	delete "$INSTDIR\pdf\*"

	# uninstall dir
	rmdir "$INSTDIR\pdf"

	# uninstall Start Menu launcher shortcuts
	delete "$SMPROGRAMS\${APPNAME}\hashdb Users Manual.lnk"
	delete "$SMPROGRAMS\${APPNAME}\uninstall ${APPNAME}.lnk"
	rmDir "$SMPROGRAMS\${APPNAME}"

	# delete the uninstaller
	delete "$INSTDIR\uninstall.exe"
 
	# Try to remove the install directory
	rmDir "$INSTDIR"
 
	# Remove uninstaller information from the registry
	DeleteRegKey HKLM "${REG_SUB_KEY}"

        # remove associated search paths from the PATH environment variable
        # were both installed
        ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR"
sectionEnd

