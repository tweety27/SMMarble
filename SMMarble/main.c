//
//  main.c
//  SMMarble
//
//  Created by Seoyeon kim.
//

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "smm_object.h"
#include "smm_database.h"
#include "smm_common.h"

#define BOARDFILEPATH "marbleBoardConfig.txt"
#define FOODFILEPATH "marbleFoodConfig.txt"
#define FESTFILEPATH "marbleFestivalConfig.txt"

//board configuration parameters
static int board_nr;
static int food_nr;
static int festival_nr;

//player parameters
static int player_nr;

//node definition for player status
typedef struct player {
        int energy;
        int position;
        char name[MAX_CHARNAME];
        int accumCredit;
        int flag_graduate;
        char takenLectures[MAX_TAKENLECTURE][MAX_CHARNAME]; // 이전에 수강한 강의 목록 저장
        int numTakenLectures; //현재까지 들은 강의 수
} player_t;

//pointer for current player status
static player_t *cur_player;

//function prototypes
int isGraduated(int player); //check if any player is graduated
void goForward(int player, int step); //make player go "step" steps on the board (check if player is graduated)
void printPlayerStatus(void); //print all player status at the beginning of each turn
float calcAverageGrade(int player); //calculate average grade of the player
smmObjGrade_e takeLecture(int player, char *lectureName, int credit); //take the lecture (insert a grade of the player)
//void* findGrade(int player, char *lectureName); //find the grade from the player's grade history
void printGrades(int player); //print all the grade history of the player


// take the lecture (insert a grade of the player)
smmObjGrade_e takeLecture(int player, char *lectureName, int credit) {
    void *boardPtr = smmdb_getData(LISTNO_NODE, cur_player[player].position);
    int takeOrDrop = 1; // 수강할지 말지 선택하는 값을 저장하기 위한 변수

    // 현재 에너지가 소요에너지 이상 있고 이전에 듣지 않은 강의이면 수강 가능, 수강 혹은 드랍 선택하는 함수
    if (cur_player[player].energy >= smmObj_getNodeEnergy(boardPtr)) {
        for (int i = 0; i < cur_player[player].numTakenLectures; ++i) {
            if (strcmp(cur_player[player].takenLectures[i], lectureName) == 0) {
                printf("이미 수강한 강의 입니다.\n");
                return 0; // 이미 수강한 강의일 경우에는 바로 return
            }
        }

        do {
            printf("수강하시려면 1 드랍하시려면 0을 눌러주세요: ");
            scanf("%d", &takeOrDrop);

            // 입력 버퍼 정리
            while(getchar() != '\n');

            // 수강 또는 드랍 여부를 판단한 후 크레딧과 에너지가 변화, 성적 랜덤 생성
            if (takeOrDrop == 1) {
                // 크레딧 증가, 에너지 감소
                cur_player[player].accumCredit += smmObj_getNodeCredit(boardPtr);
                cur_player[player].energy -= smmObj_getNodeEnergy(boardPtr);

                // 성적 랜덤 생성
                int randomGradeIndex = rand() % MAX_GRADE;
                smmObjGrade_e randomGrade = (smmObjGrade_e)randomGradeIndex;

                void *gradePtr = smmObj_genObject(lectureName, smmObjType_grade, 0, credit, 0, randomGrade);
                smmdb_addTail(LISTNO_OFFSET_GRADE + player, gradePtr);

                // 수강한 강의 목록에 추가
                strcpy(cur_player[player].takenLectures[cur_player[player].numTakenLectures], lectureName);
                cur_player[player].numTakenLectures++;

                return randomGrade;
            }
        } while (takeOrDrop != 0);
    } else {
        printf("에너지가 부족합니다.\n");
    }

    // 에너지가 부족하거나 이미 수강한 강의일 경우에는 크레딧과 에너지가 변하지 않음
    return 0;
}


// calculate average grade of the player
float calcAverageGrade(int player) {
    float totalGrade = 0.0;
    int numGrades = smmdb_len(LISTNO_OFFSET_GRADE + player);

    // 들은 강의가 없다면 유효 성적이 없다는 문구 출력
    if (numGrades == 0) {
        printf("아직 수강한 과목이 없어 유효한 성적이 존재하지 않습니다.\n");
        return 0.0;
    }

    // for문을 이용한 들은 강의 평균 성적 계산
    for (int i = 0; i < numGrades; ++i) {
        void *gradePtr = smmdb_getData(LISTNO_OFFSET_GRADE + player, i);
        totalGrade += smmObj_getNodeGrade(gradePtr);
    }

    return totalGrade / numGrades;
}

//check if any player is graduated
int isGraduated(int player)
{
    int i;
    void *gradePtr;

    for(i=0;i<player_nr;++i)
    {
        //수강 학점이 졸업 학점 보다 높고 집(0번 노드)으로 이동한 경우 게임 종료
        if(cur_player[i].accumCredit >= GRADUATE_CREDIT && cur_player[player].position == 0)
        {
            printf("\n====================Game Over====================\n");
            printf("-----------------%s has graduated-----------------\n", cur_player[i].name);

            printf("\n-----------%s's Graduation information-----------\n", cur_player[i].name);

            for(i=0;i<smmdb_len(LISTNO_OFFSET_GRADE + player);++i)
            {
                gradePtr = smmdb_getData(LISTNO_OFFSET_GRADE + player, i);
                //졸업한 플레이어가 수강한 강의의 이름, 학점, 성적 출력
                printf("%s Credit: %d, Grade: %d\n", smmObj_getNodeName(gradePtr), smmObj_getNodeCredit(gradePtr), smmObj_getNodeGrade(gradePtr));
            }
            // 평균 성적 출력
            printf("\n수강한 강의의 평균 성적은 %.2f입니다.", calcAverageGrade(player));
            exit(0);
        }
    }
    return 0;
}

//print all the grade history of the player
void printGrades(int player)
{
    int i;
    void *gradePtr;
    for (i=0;i<smmdb_len(LISTNO_OFFSET_GRADE + player);i++)
    {
        gradePtr = smmdb_getData(LISTNO_OFFSET_GRADE + player, i);
        printf("%s : %i\n", smmObj_getNodeName(gradePtr), smmObj_getNodeGrade(gradePtr));
    }
}

//print all player status at the beginning of each turn
void printPlayerStatus(void)
{
    int i;
     
    for (i=0;i<player_nr;i++)
    {
        printf("%s : credit %i, energy %i, position %i\n", 
                    cur_player[i].name,
                    cur_player[i].accumCredit,
                    cur_player[i].energy,
                    cur_player[i].position);
    }
}

//generate a new player
void generatePlayers(int n, int initEnergy)
{
    int i;
    //n time loop
    for (i=0;i<n;i++)
    {
        //input name
        printf("Input player %i's name:", i); //get player's name
        scanf("%s", cur_player[i].name);
        fflush(stdin);
         
        //set position
        //player_position[i] = 0;
        cur_player[i].position = 0;
         
        //set energy
        //player_energy[i] = initEnergy;
        cur_player[i].energy = initEnergy;
        cur_player[i].accumCredit = 0;
        cur_player[i].flag_graduate = 0;
    }
}

//rolling die
int rolldie(int player)
{
    char c;
    printf(" Press any key to roll a die (press g to see grade): ");
    c = getchar();
    fflush(stdin);

    if (c == 'g')
        printGrades(player); //print grade history of the player

    
    return (rand()%MAX_DIE + 1);
}

//action code when a player stays at a node
void actionNode(int player)
{
    void *boardPtr = smmdb_getData(LISTNO_NODE, cur_player[player].position );
    //int type = smmObj_getNodeType( cur_player[player].position );
    int type = smmObj_getNodeType( boardPtr );
    char *name = smmObj_getNodeName( boardPtr );
    
    switch(type)
    {
        //case lecture:
        case SMMNODE_TYPE_LECTURE:
            takeLecture(player, name, smmObj_getNodeCredit( boardPtr ));
            break;
        
        //case home
        case SMMNODE_TYPE_HOME:
            // 집 노드일 경우 에너지 추가
            cur_player[player].energy += smmObj_getNodeEnergy(boardPtr);
            break;
        
        //case restaurant
        case SMMNODE_TYPE_RESTAURANT:
            // 식당 노드일 경우 에너지 추가
            cur_player[player].energy += smmObj_getNodeEnergy(boardPtr);
            break;

        //case foodchande
        case SMMNODE_TYPE_FOODCHANCE:
        {
            // 음식 카드 리스트에서 랜덤으로 음식 선택
            int randomFoodIndex = rand() % food_nr;
            void* foodCardObj = smmdb_getData(LISTNO_FOODCARD, randomFoodIndex);

            // 보충 에너지를 가져와서 현재 에너지 추가
            int replenishEnergy = smmObj_getNodeEnergy(foodCardObj);
            cur_player[player].energy += replenishEnergy;

            // 선택된 음식 카드 정보 출력
            char* foodCardName = smmObj_getNodeName(foodCardObj);
            printf("먹은 음식은 %s이고, 충전된 에너지는 %d입니다.\n", foodCardName, replenishEnergy);

            break;
        }

        default:
            break;
    }
}

//make player go "step" steps on the board (check if player is graduated)
void goForward(int player, int step)
{
    void *boardPtr;
    //마지막 노드를 만나도 다시 0번 노드로 돌아가게 하기 위한 코드
    cur_player[player].position = (cur_player[player].position + step) % board_nr;
    boardPtr = smmdb_getData(LISTNO_NODE, cur_player[player].position );
     
    printf("%s go to node %i (name: %s)\n", 
            cur_player[player].name, cur_player[player].position,
            smmObj_getNodeName(boardPtr));
}


int main(void) {
    
    FILE* fp;
    char name[MAX_CHARNAME];
    int type;
    int credit;
    int energy;
    int i;
    int initEnergy;
    int turn=0;
    
    
    board_nr = 0;
    food_nr = 0;
    festival_nr = 0;
    
    srand(time(NULL));
    
    
    //1. import parameters ---------------------------------------------------------------------------------
    //1-1. boardConfig 
    if ((fp = fopen(BOARDFILEPATH,"r")) == NULL)
    {
        printf("[ERROR] failed to open %s. This file should be in the same directory of SMMarble.exe.\n", BOARDFILEPATH);
        getchar();
        return -1;
    }
    
    printf("Reading board component......\n");
    while ( fscanf(fp, "%s %i %i %i", name, &type, &credit, &energy) == 4 ) //read a node parameter set
    {
        //store the parameter set
        //(char* name, smmObjType_e objType, int type, int credit, int energy, smmObjGrade_e grade)
        void* smmObj_genObject(char* name, smmObjType_e objType, int type, int credit, int energy, smmObjGrade_e grade);
        smmObject_t* boardObj = (smmObject_t*)smmObj_genObject(name, smmObjType_board, type, credit, energy, 0);
        smmdb_addTail(LISTNO_NODE, boardObj);
        if (type == SMMNODE_TYPE_HOME)
           initEnergy = energy;
        board_nr++;
    }
    fclose(fp);
    printf("Total number of board nodes : %i\n", board_nr);
    
    
    for (i = 0;i<board_nr;i++)
    {
        void *boardObj = smmdb_getData(LISTNO_NODE, i);
        
        printf("node %i : %s, %i(%s), credit %i, energy %i\n", 
                     i, smmObj_getNodeName(boardObj), 
                     smmObj_getNodeType(boardObj), smmObj_getTypeName(smmObj_getNodeType(boardObj)),
                     smmObj_getNodeCredit(boardObj), smmObj_getNodeEnergy(boardObj));
    }

    
    //2. food card config 
    if ((fp = fopen(FOODFILEPATH,"r")) == NULL)
    {
        printf("[ERROR] failed to open %s. This file should be in the same directory of SMMarble.exe.\n", FOODFILEPATH);
        return -1;
    }
    
    printf("\n\nReading food card component......\n");
    while (fscanf(fp, "%s %i", name, &energy) == 2) //read a food parameter set
    {
        //store the parameter set
        void* foodCardObj = smmObj_genObject(name, smmObjType_card, 0, 0, energy, 0);
        smmdb_addTail(LISTNO_FOODCARD, foodCardObj);
        food_nr++;
    }
    fclose(fp);
    printf("Total number of food cards : %i\n", food_nr);
    
    for (i = 0;i<food_nr;i++)
    {
        void *foodCardObj = smmdb_getData(LISTNO_FOODCARD, i);
        
        printf("node %i : %s, energy %i\n", 
                     i, smmObj_getNodeName(foodCardObj), 
                     smmObj_getNodeEnergy(foodCardObj));
    }
    
    //3. festival card config 
    if ((fp = fopen(FESTFILEPATH,"r")) == NULL)
    {
        printf("[ERROR] failed to open %s. This file should be in the same directory of SMMarble.exe.\n", FESTFILEPATH);
        return -1;
    }
    
    printf("\n\nReading festival card component......\n");
    while (fscanf(fp, "%s ", name) == 1) //read a festival card string
    {
        //store the parameter set
        void* festCardObj = smmObj_genObject(name, smmObjType_card, 0, 0, 0, 0);
        smmdb_addTail(LISTNO_FESTCARD, festCardObj);
        festival_nr++;
    }
    fclose(fp);
    printf("Total number of festival cards : %i\n", festival_nr);
    
    for (i = 0;i<festival_nr;i++)
    {
        void *festCardObj = smmdb_getData(LISTNO_FESTCARD, i);
        
        printf("node %i : %s\n", 
                     i, smmObj_getNodeName(festCardObj));
    }

    //2. Player configuration ---------------------------------------------------------------------------------
    
    do
    {
        //input player number to player_nr
        printf("\n====================Game Start====================\n");
        printf("input player no.:");
        scanf("%d", &player_nr);
        fflush(stdin);
    }
    while (player_nr < 0 || player_nr >  MAX_PLAYER);
    
    cur_player = (player_t*)malloc(player_nr*sizeof(player_t));
    generatePlayers(player_nr, initEnergy);
    
    //3. SM Marble game starts ---------------------------------------------------------------------------------
    while (isGraduated(turn) == 0) //is anybody graduated?
    {
        int die_result;
        
        
        //4-1. initial printing
        printPlayerStatus();
        
        //4-2. die rolling (if not in experiment)        
        die_result = rolldie(turn);
        
        //4-3. go forward
        goForward(turn, die_result);

		//4-4. take action at the destination node of the board
        actionNode(turn);
        
        //4-5. next turn
        turn = (turn + 1)%player_nr;
    }
    
    
    free(cur_player);
    system("PAUSE");
    return 0;
}
