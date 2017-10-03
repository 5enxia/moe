#include"moe.h"

void startCurses(){

  int h,
      w;

  initscr();      // start terminal contorl
  curs_set(1);    // set cursr
  keypad(stdscr, TRUE);   // enable cursr keys

  getmaxyx(stdscr, h, w);     // set window size

  start_color();      // color settings
  init_pair(1, COLOR_WHITE, COLOR_BLACK);     // set color strar is white and back is black
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_CYAN, COLOR_BLACK);

  erase();  	// screen display

//  setlocale(LC_ALL, "");

  ESCDELAY = 25;    // delete esc key time lag

/*
  WINDOW *windows[WIN_NUM];
  if(signal(SIGINT, signal_handler) == SIG_ERR ||
    signal(SIGQUIT, signal_handler) == SIG_ERR){
      fprintf(stderr, "signal failure\n");
      exit(EXIT_FAILURE);
  }

  if(initscr() == NULL){
    fprintf(stderr, "initscr failure\n");
    exit(EXIT_FAILURE);
  }
*/

  move(0, 0);     // set cursr point
}

void exitCurses(){
 endwin(); 
}

void editorStatInit(editorStat* stat){

//  stat->mode = 1;
  stat->currentLine = 0;
  stat->numOfLines = 0;
  stat->lineDigit = 3;    // 3 is default line digit
  stat->lineDigitSpace = stat->lineDigit + 1;
  stat->y = 0;
  stat->x = 0;
  strcpy(stat->filename, "test_new.txt");
}

int writeFile(gapBuffer* gb, editorStat *stat){
  
  FILE *fp;
//  char *filename = "test_new.txt";

  if ((fp = fopen(stat->filename, "w")) == NULL) {
    printf("%s Cannot file open... \n", stat->filename);
      return -1;
    }
  
  for(int i=0; i < gb->size; i++){
    fputs(gapBufferAt(gb, i)->elements, fp);
    if(i != gb->size -1 ) fputc('\n', fp);
  }

  fclose(fp);

  return 0;
}

int countLineDigit(int numOfLines){

  int lineDigit = 0;
  while(numOfLines > 0){
    numOfLines /= 10;
    lineDigit++;
  }
  return lineDigit;
}

void printLineNum(int lineDigit, int currentLine, int y){

  int lineDigitSpace = lineDigit - countLineDigit(currentLine + 1);
  move(y, 0);
  for(int i=0; i<lineDigitSpace; i++) mvprintw(y, i, " ");
  bkgd(COLOR_PAIR(2));
  printw("%d:", currentLine + 1); 
}

void printLine(gapBuffer* gb, int lineDigit, int currentLine, int y){

  printLineNum(lineDigit, currentLine, y);
  bkgd(COLOR_PAIR(1));
  mvprintw(y, lineDigit + 1, "%s", gapBufferAt(gb, currentLine)->elements);
}

int keyUp(gapBuffer* gb, editorStat* stat){
  if(stat->currentLine == 0) return 0;
        
  if(stat->y == 0){
    stat->currentLine--;
    wscrl(stdscr, -1);    // scroll
    printLine(gb, stat->lineDigit,  stat->currentLine, stat->y);
    stat->x = gapBufferAt(gb, stat->currentLine)->numOfChar + stat->lineDigitSpace - 1;
    return 0;
  }else if(COLS - stat->lineDigitSpace - 1 <= gapBufferAt(gb, stat->currentLine - 1)->numOfChar){
    stat->y -= 2;
    stat->currentLine--;
    return 0;
  }

  stat->y--;
  stat->currentLine--;
  if(stat->x > stat->lineDigit + gapBufferAt(gb, stat->currentLine)->numOfChar + 1)
    stat->x = stat->lineDigit + gapBufferAt(gb, stat->currentLine)->numOfChar + 1;
  return 0;
}

int keyDown(gapBuffer* gb, editorStat* stat){
  if(stat->currentLine + 1 == gb->size) return 0;
        
  if(stat->y >= LINES -1){
    stat->currentLine++;
    wscrl(stdscr, 1);
    move(LINES-1, 0);
    printLine(gb, stat->lineDigit, stat->currentLine, stat->y);
    stat->x = gapBufferAt(gb, stat->currentLine)->numOfChar + stat->lineDigitSpace- 1;
    return 0;
  }else if(COLS - stat->lineDigitSpace - 1 <= gapBufferAt(gb, stat->currentLine + 1)->numOfChar){
    stat->y += 2;
    stat->currentLine++;
    return 0;
  }
  stat->y++;
  stat->currentLine++;
  if(stat->x > stat->lineDigitSpace + gapBufferAt(gb, stat->currentLine)->numOfChar)
    stat->x = stat->lineDigit + gapBufferAt(gb, stat->currentLine)->numOfChar + 1;
  return 0;
}

int keyRight(gapBuffer* gb, editorStat* stat){

  if(stat->x >= gapBufferAt(gb, stat->currentLine)->numOfChar + stat->lineDigitSpace) return 0;
  stat->x++;
  return 0;
}

int keyLeft(gapBuffer* gb, editorStat* stat){

  if(stat->x == stat->lineDigit + 1) return 0;
  stat->x--;
  return 0;
}

int keyBackSpace(gapBuffer* gb, editorStat* stat){
  if(stat->y == 0 && stat->x == stat->lineDigitSpace) return 0;
  stat->x--;
  move(stat->y, stat->x);
  delch();
  if(stat->x < stat->lineDigitSpace && gapBufferAt(gb, stat->currentLine)->numOfChar > 0){
    int tmpNumOfChar = gapBufferAt(gb, stat->currentLine - 1)->numOfChar;
    for(int i=0; i<gapBufferAt(gb, stat->currentLine)->numOfChar; i++) {
      charArrayPush(gapBufferAt(gb, stat->currentLine - 1), gapBufferAt(gb, stat->currentLine)->elements[i]);
    }
    gapBufferDel(gb, stat->currentLine, stat->currentLine + 1);
    stat->numOfLines--;
    deleteln();
    move(stat->y - 1, stat->x);
    for(int i=stat->y - 1; i<stat->numOfLines; i++){
      if(i == LINES - 1) return 0;
      printLine(gb, stat->lineDigit, i, i);
    }
    stat->y--;
    stat->x = stat->lineDigitSpace + tmpNumOfChar;
    stat->currentLine--;
    return 0;
  }

  charArrayDel(gapBufferAt(gb, stat->currentLine), (stat->x - stat->lineDigitSpace));
  if(stat->x < stat->lineDigitSpace && gapBufferAt(gb, stat->currentLine)->numOfChar == 0){
    gapBufferDel(gb, stat->currentLine, stat->currentLine + 1);
    deleteln();
    stat->numOfLines--;
    for(int i=stat->currentLine; i < gb->size-1; i++){
      if(i == LINES-1) return 0;
        printLine(gb, stat->lineDigit, i, i);
        printw("\n");
      }
    stat->currentLine--;
    if(stat->y > 0) stat->y--;
    stat->x = gapBufferAt(gb, stat->currentLine)->numOfChar + stat->lineDigitSpace ;
  }
  return 0;
}

int keyEnter(gapBuffer* gb, editorStat* stat){
  stat->numOfLines++;
  if(stat->y == LINES - 1){
    stat->currentLine++;
    if(stat->x == stat->lineDigitSpace){
      {
        charArray* ca = (charArray*)malloc(sizeof(charArray));
        charArrayInit(ca);
        gapBufferInsert(gb, ca, stat->currentLine);
        charArrayPush(gapBufferAt(gb, stat->currentLine), '\0');
      }
      wscrl(stdscr, 1);
      printLine(gb, stat->lineDigit, stat->currentLine, LINES -1);
      stat->x = gapBufferAt(gb, stat->currentLine)->numOfChar + stat->lineDigitSpace - 1;
    }else{
      {
        charArray* ca = (charArray*)malloc(sizeof(charArray));
        charArrayInit(ca);
        gapBufferInsert(gb, ca, stat->currentLine + 1);
        charArrayPush(gapBufferAt(gb, stat->currentLine + 1), '\0');
        int tmp = gapBufferAt(gb, stat->currentLine)->numOfChar;
        for(int i = 0; i < tmp - (stat->x - stat->lineDigitSpace); i++){
          charArrayInsert(gapBufferAt(gb, stat->currentLine + 1), gapBufferAt(gb, stat->currentLine)->elements[i + stat->x - stat->lineDigitSpace], i);
          gapBufferAt(gb, stat->currentLine)->numOfChar--;
        }
        for(int i=0; i < tmp - (stat->x - stat->lineDigitSpace); i++) charArrayPop(gapBufferAt(gb, stat->currentLine));
      }
      wscrl(stdscr, 1);
      move(LINES - 2, stat->x);
      deleteln();
      printLine(gb, stat->lineDigit, stat->currentLine, LINES - 2);
      printLine(gb, stat->lineDigit, stat->currentLine + 1, LINES - 1);
      stat->x = gapBufferAt(gb, stat->currentLine)->numOfChar + stat->lineDigitSpace - 1;
      stat->currentLine++;
    }
    return 0;
  }

  insertln();
  if(stat->x == stat->lineDigitSpace){
    {
      charArray* ca = (charArray*)malloc(sizeof(charArray));
      charArrayInit(ca);
      gapBufferInsert(gb, ca, stat->currentLine);
      charArrayPush(gapBufferAt(gb, stat->currentLine), '\0');
      gapBufferAt(gb, stat->currentLine)->numOfChar++;
    }
    gapBufferAt(gb, stat->currentLine)->numOfChar--;
    for(int i=stat->currentLine; i < gb->size - 1; i++){
      if(i == LINES - 1) break;
        printLine(gb, stat->lineDigit, i, i);
        printw("\n");
    }
    stat->currentLine++;
    stat->y++;
    // Up lineDigit
    if(countLineDigit(stat->numOfLines) > countLineDigit(stat->numOfLines - 1)){
      stat->lineDigit = countLineDigit(stat->numOfLines);
      clear();
      for(int i=0; i<gb->size-1; i++){
        if(i == LINES-1) break;
          printLine(gb, stat->lineDigit, i, i);
          printw("\n");
      }
    }
    return 0;
  }else{
    {
      charArray* ca = (charArray*)malloc(sizeof(charArray));
      charArrayInit(ca);
      gapBufferInsert(gb, ca, stat->currentLine + 1);
      charArrayPush(gapBufferAt(gb, stat->currentLine + 1), '\0');
      int tmp = gapBufferAt(gb, stat->currentLine)->numOfChar;
      for(int i = 0; i < tmp - (stat->x - stat->lineDigitSpace); i++){
        charArrayInsert(gapBufferAt(gb, stat->currentLine + 1), gapBufferAt(gb, stat->currentLine)->elements[i + stat->x - stat->lineDigitSpace], i);
        gapBufferAt(gb, stat->currentLine)->numOfChar--;
      }
      for(int i=0; i < tmp - (stat->x - stat->lineDigitSpace); i++) charArrayPop(gapBufferAt(gb, stat->currentLine));
    }
    stat->x = stat->lineDigitSpace;
    for(int i=stat->currentLine; i < gb->size-1; i++){
      if(i == LINES-1) break;
        printLine(gb, stat->lineDigit, i, i);
        printw("\n");
    }
    stat->currentLine++;
    stat->y++;
    // Up lineDigit
    if(countLineDigit(stat->numOfLines) > countLineDigit(stat->numOfLines - 1)){
      stat->lineDigit = countLineDigit(stat->numOfLines);
      clear();
      for(int i=0; i<gb->size-1; i++){
        if(i == LINES-1) break;
          printLine(gb, stat->lineDigit, i, i);
          printw("\n");
      }
    }
    return 0;
  }
}

void insertMode(gapBuffer* gb, editorStat* stat){

  int key;

  stat->y = 0;
  stat->x = stat->lineDigitSpace;
  stat->currentLine = 0;

  while(1){

    move(stat->y, stat->x);
    refresh();
    noecho();
    key = getch();

    if(key == KEY_ESC){
      writeFile(gb, stat);
      exitCurses();
      break;
    } 
/*
    if(key == KEY_ESC) {
      nomalMode(gb, lineDigit, lineNum);
    }
*/

    switch(key){

      case KEY_UP:
        keyUp(gb, stat);
        break;

      case KEY_DOWN:
        keyDown(gb, stat);
        break;
        
      case KEY_RIGHT:
        keyRight(gb, stat);
        break;

      case KEY_LEFT:
        keyLeft(gb, stat);
        break;
        
      case KEY_BACKSPACE:
        keyBackSpace(gb, stat);
        break;

      case 10:    // 10 is Enter key
        keyEnter(gb, stat);
        break;
      
      default:
        echo();
        charArrayInsert(gapBufferAt(gb, stat->currentLine), key, stat->x - stat->lineDigitSpace);
        insch(key);
        stat->x++;
    }
  }
}

int openFile(char* filename){

  FILE *fp = fopen(filename, "r");
  if(fp == NULL){
		printf("%s Cannot file open... \n", filename);
		exit(0);
  }

  gapBuffer* gb = (gapBuffer*)malloc(sizeof(gapBuffer));
  gapBufferInit(gb);
  
  {
    charArray* ca = (charArray*)malloc(sizeof(charArray));
    charArrayInit(ca);
    gapBufferInsert(gb, ca, 0);
  }

  editorStat* stat = (editorStat*)malloc(sizeof(editorStat));
  editorStatInit(stat);

  char  ch;
  while((ch = fgetc(fp)) != EOF){
    if(ch=='\n'){
      stat->currentLine += 1;
      charArray* ca = (charArray*)malloc(sizeof(charArray));
      charArrayInit(ca);
      gapBufferInsert(gb, ca, stat->currentLine);
    }else charArrayPush(gapBufferAt(gb, stat->currentLine), ch);
  }
  fclose(fp);

  startCurses();

  if(stat->lineDigit < countLineDigit(stat->currentLine + 1)) stat->lineDigit = countLineDigit(stat->currentLine + 1);

  stat->numOfLines = stat->currentLine + 1;
  for(int i=0; i < stat->numOfLines; i++){
    if(i == LINES) break;
    printLine(gb, stat->lineDigit, i, i);
    printw("\n");
  }

  scrollok(stdscr, TRUE);			// enable scroll
  insertMode(gb, stat);

  return 0;
}

int newFile(){

  gapBuffer* gb = (gapBuffer*)malloc(sizeof(gapBuffer));
  gapBufferInit(gb);
  {
    charArray* ca = (charArray*)malloc(sizeof(charArray));
    charArrayInit(ca);
    gapBufferInsert(gb, ca, 0);
  }

  editorStat* stat = (editorStat*)malloc(sizeof(editorStat));
  editorStatInit(stat);


  startCurses(); 
  printLine(gb, stat->lineDigit, 0, 0);
  scrollok(stdscr, TRUE);			// enable scroll
  insertMode(gb, stat);

  return 0;
}

int main(int argc, char* argv[]){

  if(argc < 2){
    newFile();
  }

  openFile(argv[1]);

  return 0;
}
