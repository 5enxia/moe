import ui, strutils, strformat, os, osproc
import syntax/highlite
import bufferstatus, color, unicodetext, settings, window, gapbuffer

type StatusBar* = object
  window*: Window
  windowIndex*: int
  bufferIndex*: int

proc initStatusBar*(): StatusBar {.inline.} =
  const
    h = 1
    w = 1
    t = 1
    l = 1
    color = EditorColorPair.defaultChar

  result.window = initWindow(h, w, t, l, color)

proc showFilename(mode, prevMode: Mode): bool {.inline.} =
  not isHistoryManagerMode(mode, prevMode) and
  not isConfigMode(mode, prevMode)

proc writeStatusBarNormalModeInfo(bufStatus: var BufferStatus,
                                  statusBar: var StatusBar,
                                  statusBarBuffer: var seq[Rune],
                                  windowNode: WindowNode,
                                  isActiveWindow: bool,
                                  settings: EditorSettings) =

  proc setStatusBarColor(mode: Mode): EditorColorPair =
    case mode:
      of Mode.insert:
        if isActiveWindow: return EditorColorPair.statusBarInsertMode
        else: return EditorColorPair.statusBarInsertModeInactive
      of Mode.visual:
        if isActiveWindow: return EditorColorPair.statusBarVisualMode
        else: return EditorColorPair.statusBarVisualModeInactive
      of Mode.replace:
        if isActiveWindow: return EditorColorPair.statusBarReplaceMode
        else: return EditorColorPair.statusBarReplaceModeInactive
      of Mode.ex:
        if isActiveWindow: return EditorColorPair.statusBarExMode
        else: return EditorColorPair.statusBarExModeInactive
      else:
        if isActiveWindow: return EditorColorPair.statusBarNormalMode
        else: return EditorColorPair.statusBarNormalModeInactive

  let
    mode = bufStatus.mode
    prevMode = bufStatus.prevMode
    color = setStatusBarColor(mode)
    statusBarWidth = statusBar.window.width

  statusBarBuffer.add(ru" ")
  statusBar.window.append(ru" ", color)

  if settings.statusBar.filename:
    var filename = if not showFilename(mode, prevMode): ru""
                   elif bufStatus.path.len > 0: bufStatus.path
                   else: ru"No name"
    let homeDir = ru(getHomeDir())
    if (filename.len() >= homeDir.len() and
        filename[0..homeDir.len()-1] == homeDir):
      filename = filename[homeDir.len()-1..filename.len()-1]
      if filename[0] == ru'/':
        filename = ru"~" & filename
      else:
        filename = ru"~/" & filename
    statusBarBuffer.add(filename)
    statusBar.window.append(filename, color)

  if bufStatus.countChange > 0 and settings.statusBar.chanedMark:
    statusBarBuffer.add(ru" [+]")
    statusBar.window.append(ru" [+]", color)

  if statusBarWidth - statusBarBuffer.len < 0: return
  statusBar.window.append(ru " ".repeat(statusBarWidth - statusBarBuffer.len), color)

  let
    line = if settings.statusBar.line:
             fmt"{windowNode.currentLine + 1}/{bufStatus.buffer.len}"
           else: ""
    column = if settings.statusBar.column:
               fmt"{windowNode.currentColumn + 1}/{bufStatus.buffer[windowNode.currentLine].len}"
             else: ""
    encoding = if settings.statusBar.characterEncoding: $bufStatus.characterEncoding
               else: ""
    language = if bufStatus.language == SourceLanguage.langNone: "Plain"
               else: sourceLanguageToStr[bufStatus.language]
    info = fmt"{line} {column} {encoding} {language} "
  statusBar.window.write(0, statusBarWidth - info.len, info, color)

proc writeStatusBarFilerModeInfo(bufStatus: var BufferStatus,
                                 statusBar: var StatusBar,
                                 statusBarBuffer: var seq[Rune],
                                 windowNode: WindowNode,
                                 isActiveWindow: bool,
                                 settings: EditorSettings) =

  let
    color = if isActiveWindow: EditorColorPair.statusBarFilerMode
            else: EditorColorPair.statusBarFilerModeInactive
    statusBarWidth = statusBar.window.width

  if settings.statusBar.directory: statusBar.window.append(ru" ", color)
  statusBar.window.append(getCurrentDir().toRunes, color)
  statusBar.window.append(ru " ".repeat(statusBarWidth - 5), color)

proc writeStatusBarBufferManagerModeInfo(bufStatus: var BufferStatus,
                                         statusBar: var StatusBar,
                                         statusBarBuffer: var seq[Rune],
                                         windowNode: WindowNode,
                                         isActiveWindow: bool,
                                         settings: EditorSettings) =

  let
    color = if isActiveWindow: EditorColorPair.statusBarNormalMode
            else: EditorColorPair.statusBarNormalModeInactive
    info = fmt"{windowNode.currentLine + 1}/{bufStatus.buffer.len - 1}"
    statusBarWidth = statusBar.window.width

  statusBar.window.append(ru " ".repeat(statusBarWidth - statusBarBuffer.len),
                                        color)
  statusBar.window.write(0, statusBarWidth - info.len - 1, info, color)

proc writeStatusBarLogViewerModeInfo(bufStatus: var BufferStatus,
                                  statusBar: var StatusBar,
                                  statusBarBuffer: var seq[Rune],
                                  windowNode: WindowNode,
                                  isActiveWindow: bool,
                                  settings: EditorSettings) =

  let
    color = if isActiveWindow: EditorColorPair.statusBarNormalMode
            else: EditorColorPair.statusBarNormalModeInactive
    info = fmt"{windowNode.currentLine + 1}/{bufStatus.buffer.len - 1}"
    statusBarWidth = statusBar.window.width

  statusBar.window.append(ru " ".repeat(statusBarWidth - statusBarBuffer.len),
                          color)
  statusBar.window.write(0, statusBarWidth - info.len - 1, info, color)

proc writeStatusBarCurrentGitBranchName(statusBar: var StatusBar,
                                        statusBarBuffer: var seq[Rune],
                                        isActiveWindow: bool) =

  # Get current git branch name
  let cmdResult = execCmdEx("git rev-parse --abbrev-ref HEAD")
  if cmdResult.exitCode != 0: return

  let
    branchName = cmdResult.output
    ## Add symbol and delete newline
    buffer = ru"  " & branchName[0 .. branchName.high - 1].toRunes & ru" "
    color = EditorColorPair.statusBarGitBranch

  statusBarBuffer.add(buffer)
  statusBar.window.append(buffer, color)

proc setModeStr(mode: Mode, isActiveWindow, showModeInactive: bool): string =
  if not isActiveWindow and not showModeInactive: result = ""
  else:
    case mode:
    of Mode.insert: result = " INSERT "
    of Mode.visual, Mode.visualBlock: result = " VISUAL "
    of Mode.replace: result = " REPLACE "
    of Mode.filer: result = " FILER "
    of Mode.bufManager: result = " BUFFER "
    of Mode.ex: result = " EX "
    of Mode.logViewer: result = " LOG "
    of Mode.recentFile: result = " RECENT "
    of Mode.quickRun: result = " QUICKRUN "
    of Mode.history: result = " HISTORY "
    of Mode.diff: result = "DIFF "
    of Mode.config: result = " CONFIG "
    of Mode.debug: result = " DEBUG "
    else: result = " NORMAL "

proc setModeStrColor(mode: Mode): EditorColorPair =
  case mode
    of Mode.insert: return EditorColorPair.statusBarModeInsertMode
    of Mode.visual: return EditorColorPair.statusBarModeVisualMode
    of Mode.replace: return EditorColorPair.statusBarModeReplaceMode
    of Mode.filer: return EditorColorPair.statusBarModeFilerMode
    of Mode.ex: return EditorColorPair.statusBarModeExMode
    else: return EditorColorPair.statusBarModeNormalMode

proc isShowGitBranchName(mode, prevMode: Mode,
                         isActiveWindow: bool,
                         settings: EditorSettings): bool =

  if settings.statusBar.gitbranchName:
    let showGitInactive = settings.statusBar.showGitInactive

    if showGitInactive or
    (not showGitInactive and isActiveWindow): result = true

  if mode == Mode.normal or
     mode == Mode.insert or
     mode == Mode.visual or
     mode == Mode.replace: result = true
  elif mode == Mode.ex:
    if prevMode == Mode.normal or
       prevMode == Mode.insert or
       prevMode == Mode.visual or
       prevMode == Mode.replace: result = true
  else:
    result = false

proc writeStatusBar*(bufStatus: var BufferStatus,
                     statusBar: var StatusBar,
                     windowNode: WindowNode,
                     isActiveWindow: bool,
                     settings: EditorSettings) =

  statusBar.window.erase

  let
    currentMode = bufStatus.mode
    prevMode = bufStatus.prevMode
    color = setModeStrColor(currentMode)
    modeStr = setModeStr(currentMode,
                         isActiveWindow,
                         settings.statusBar.showModeInactive)

  var statusBarBuffer = if windowNode.x > 0: ru" " & modeStr.toRunes
                        else: modeStr.toRunes

  ## Write current mode
  if settings.statusBar.mode:
    statusBar.window.write(0, 0, statusBarBuffer, color)

  if isShowGitBranchName(currentMode, prevMode, isActiveWindow, settings):
    statusBar.writeStatusBarCurrentGitBranchName(
      statusBarBuffer,
      isActiveWindow)

  if isFilerMode(currentMode, prevMode):
    bufStatus.writeStatusBarFilerModeInfo(
      statusBar,
      statusBarBuffer,
      windowNode,
      isActiveWindow,
      settings)
  elif currentMode == Mode.bufManager:
    bufStatus.writeStatusBarBufferManagerModeInfo(
      statusBar,
      statusBarBuffer,
      windowNode,
      isActiveWindow,
      settings)
  elif currentMode == Mode.logViewer:
    bufStatus.writeStatusBarLogViewerModeInfo(
      statusBar,
      statusBarBuffer,
      windowNode,
      isActiveWindow,
      settings)
  else:
    bufStatus.writeStatusBarNormalModeInfo(
      statusBar,
      statusBarBuffer,
      windowNode,
      isActiveWindow,
      settings)

  statusBar.window.refresh
