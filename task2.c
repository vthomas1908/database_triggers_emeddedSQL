//Main task 2
//Thomas Van Klaveren
//Graduate final project CS586
//allow user to update agent_with_languages
//and use embeded SQL to update the appropriate
//language, languagerel, and agent tables
//gcc -std=c99 -L$(pg_config --libdir) -I$(pg_config --includedir) -o task2 task2.c

#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <string.h>
#include <unistd.h>

#define agent_id_idx 0
#define first_idx 1
#define middle_idx 2
#define last_idx 3
#define address_idx 4
#define city_idx 5
#define country_idx 6
#define salary_idx 7
#define clearance_id_idx 8
#define lang_id_idx 9
#define language_idx 10

//function to exit if error
void exit_nicely(PGconn *conn)
{
  PQfinish(conn);
  exit(1);
}

//function to check if begin transaction ok
void check_command(PGconn *conn, PGresult *res)
{
  if(PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    fprintf(stderr, "command failed: %s\n",
            PQerrorMessage(conn));
    PQclear(res);
    exit_nicely(conn);
  }
}


//function to check if fetch ok
void check_tuple(PGconn * conn, PGresult *res)
{
  if(PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    fprintf(stderr, "FETCH command failed: %s\n",
            PQerrorMessage(conn));
    PQclear(res);
    exit_nicely(conn);
  }
}

void check_buffer(char *string, int len)
{
  int c = 0;
  if(string[len-2] != '\0' && string[len-2] != '\n')
    while((c = getchar()) != '\n' && c != EOF)
      ;

  else if(string[len-2] == '\n')
    string[len-2] = '\0';

  else if(string[len-2] == '\0')
    string[strlen(string)-1] = '\0';
}

void clear_screen()
{
  for (int i = 0; i < 50; i++)
    printf("\n");
}

int other_updates()
{
  char y_or_n[2] = {'\0'};
  printf("\n\nAny other updates to this agent? (y or n): ");
  memset(y_or_n, '\0', 2);
  fgets(y_or_n, 2, stdin);
  check_buffer(y_or_n, 2);

  while(y_or_n[0] != 'y' && y_or_n[0] != 'n')
  {
    printf("\ny or n: ");
    fgets(y_or_n, 2, stdin);
    check_buffer(y_or_n, 2);
  }

  if(y_or_n[0] == 'n')
    return 1;
  else
    return 0;
}


int main(int argc, char** argv)
{
  char conninfo[100];
  PGconn *conn;
  PGresult *res;
  PGresult *res2;
  PGresult *res3;
  PGresult *res4;
  int c = 0; //for checking chars in stdin
  int rows = 0;
  int agent_cnt = 0;
  int lang_count = 0;
  int curr_agent = 0;
  char choice[3] = {'\0'};
  char t_f[2] = {'\0'};
  char agent_id[11] = {'\0'};
  char new_agent_id[11] = {'\0'};
  char first[21] = {'\0'};
  char middle[21] = {'\0'};
  char last[21] = {'\0'};
  char address[51] = {'\0'};
  char city[21] = {'\0'};
  char country[21] = {'\0'};
  char salary[21] = {'\0'};
  char clearance_id[11] = {'\0'};
  char lang_id[11] = {'\0'};
  char language[21] = {'\0'};
  char new_lang_id[11] = {'\0'};
  char new_language[21] = {'\0'};
  char string[500] = {'\0'};
  char test[1] = {'\0'};
  int lang_check;
  int update_done = 0;

  char info[] = "host=dbclass.cs.pdx.edu port=5432 dbname=f15ddb75 user=f15ddb75 password=";
  char pass[9] = {0x4d, 0x79, 0x6b, 0x69, 0x74, 0x74, 0x79, 0x34, '\0'};

  strncpy(conninfo, info, 80);
  strncat(conninfo, pass, 8);

  //connect to database
  conn = PQconnectdb(conninfo);
  if(PQstatus(conn) != CONNECTION_OK)
  {
    fprintf(stderr, "connection to database failed: %s",
            PQerrorMessage(conn));
    exit_nicely(conn);
  }

  //start transaction, exit if error, clear res
  res = PQexec(conn, "BEGIN");
  check_command(conn, res);
  PQclear(res);

  //create the agent_with_languages2 table
  memset(string, '\0', 500);
  strcpy(string, "create table agent_with_languages2 as ");
  strcat(string, "select a.agent_id, a.first, a.middle, a.last, ");
  strcat(string, "a.address, a.city, a.country, a.salary, a.clearance_id, ");
  strcat(string, "lr.lang_id, l.language ");
  strcat(string, "from agent a natural join languagerel lr ");
  strcat(string, "natural join language l order by a.agent_id");

  res = PQexec(conn, string);
  check_command(conn, res);
  PQclear(res);

  //set cursor for agent_with_languages2 table (no triggers attached)
  res = PQexec(conn,
    "declare main cursor for select * from agent_with_languages2 order by agent_id");
  check_command(conn, res);
  PQclear(res);

  //get count of rows
  res = PQexec(conn, "select count(*) from agent_with_languages2");
  check_tuple(conn, res);
  rows = atoi(PQgetvalue(res, 0, 0));
  PQclear(res);

  res = PQexec(conn, "fetch next from main");
  check_tuple(conn, res);

  //loop through the rows of original agent_with_languages2 tbl
  //to update/insert/delete agent_with_languages2 and manually
  //adjust apporopriate language/languagerel/agent table (no triggers)
  for(int i = 1; i < rows; i++)
  {
    //curr agent to get all languages for an agent at once and know
    //where we are in the main cursor
    curr_agent = atoi(PQgetvalue(res, 0, 0));

    //find # of languages for current agent to display langs nicely
    memset(string, '\0', 500);
    strcpy(string, "select count(*) from agent_with_languages2 ");
    strcat(string, "where agent_id = ");
    strcat(string, PQgetvalue(res, 0, 0));

    res2 = PQexec(conn, string);
    check_tuple(conn, res2);
    lang_count = atoi(PQgetvalue(res2, 0, 0));
    PQclear(res2);

    //temp cursor so we get all langs for this agent
    memset(string, '\0', 500);
    strcpy(string, "declare temp cursor for ");
    strcat(string, "select * from agent_with_languages2");
    strcat(string, " where agent_id = ");
    strcat(string, PQgetvalue(res, 0, 0));

    res2 = PQexec(conn, string);
    check_command(conn, res2);

    //display the agent info
    clear_screen();

    printf("\n\nAgent Information:\n\n");
    printf("agent_id: %s     first: %s     middle: %s     last: %s\n",
         PQgetvalue(res, 0, 0), PQgetvalue(res, 0, 1), PQgetvalue(res, 0, 2),
         PQgetvalue(res, 0, 3));
    printf("address: %s     city: %s     country: %s\n",
        PQgetvalue(res, 0, 4), PQgetvalue(res, 0, 5), PQgetvalue(res, 0, 6));
    printf("salary: %s     clearance_id: %s\n",
           PQgetvalue(res, 0, 7), PQgetvalue(res, 0, 8));
    printf("Language(id): ");

    //display all languages from res2 (temp cursor)
    for(int j = 0; j < lang_count ; j++)
    {
      PQclear(res2);
      res2 = PQexec(conn, "fetch next from temp");
      printf("%s(%s)  ",
           PQgetvalue(res2, 0, 10), PQgetvalue(res2, 0, 9));
    }

    //close the temp cursor
    PQclear(res2);
    res2 = PQexec(conn, "close temp");
    check_command(conn, res2);
    PQclear(res2);
    PQclear(res);

    //set cursor to the last row for this agent so 
    //we can use current agent data and will go to 
    //next agent on next iteration
    if(i < rows-1)
    {
      res = PQexec(conn, "fetch next from main");
      check_tuple(conn, res);
      i++;
    }

    while(atoi(PQgetvalue(res, 0, 0)) == curr_agent && i < rows-1)
    {
      PQclear(res);
      res = PQexec(conn, "fetch next from main");
      check_tuple(conn, res);
      i++;
    }

    PQclear(res);
    res = PQexec(conn, "fetch prior from main");
    i--;

    //display the options the user has for this agent
    printf("\n\nWhat would you like to do?\n");
    printf("1. Add new agent\n2. Delete this agent\n");
    printf("3. Add a language for this agent\n");
    printf("4. Delete a language for this agent\n");
    printf("5. Update this agent\n6. Move to next agent\n");
    printf("7. Done\n\n select an option: ");

    //get the users choice
    memset(choice, '\0', 2);
    fgets(choice, 2, stdin);
    check_buffer(choice, 2);

    //if user picks 1, insert new agent
    if(atoi(choice) == 1)
    {
      printf("\n\nNew agent's agent_ID: ");
      memset(agent_id, '\0', 11);
      fgets(agent_id, 11, stdin);
      check_buffer(agent_id, 11);

      //check that the entered agent_id doesn't already exist
      memset(string, '\0', 500);
      strcpy(string, "select ");
      strcat(string, agent_id);
      strcat(string, " in(select agent_id from agent_with_languages2)");

      res2 = PQexec(conn, string);
      check_tuple(conn, res2);

      memset(test, '\0', 2);
      strncpy(test, PQgetvalue(res2, 0, 0), 1);
      PQclear(res2);

      //if the agent_id already exists set agent_id to next highest
      if(test[0] == 't')
      {
        res2 = PQexec(conn, "select max(agent_id)+1 from agent");
        check_tuple(conn, res2);
        memset(agent_id, '\0', 11);
        strcpy(agent_id, PQgetvalue(res2, 0, 0));
        printf("\n\nThat agent_id already exists, agent id will be %s\n",
               agent_id);
        PQclear(res2);
      }

      //get the rest of the agent's info
      memset(first, '\0', 21);
      printf("\nNew agent's first name: ");
      fgets(first, 21, stdin);
      check_buffer(first, 21);

      memset(middle, '\0', 21);
      printf("\nNew agent's middle name: ");
      fgets(middle, 21, stdin);
      check_buffer(middle, 21);

      memset(last, '\0', 21);
      printf("\nNew agent's last name: ");
      fgets(last, 21, stdin);
      check_buffer(last, 21);

      memset(address, '\0', 51);
      printf("\nNew agent's address: ");
      fgets(address, 51, stdin);
      check_buffer(address, 51);

      memset(city, '\0', 21);
      printf("\nNew agent's city: ");
      fgets(city, 21, stdin);
      check_buffer(city, 21);

      memset(country, '\0', 21);
      printf("\nNew agent's country: ");
      fgets(country, 21, stdin);
      check_buffer(country, 21);

      memset(salary, '\0', 21);
      printf("\nNew agent's salary: ");
      fgets(salary, 21, stdin);
      check_buffer(salary, 21);

      memset(clearance_id, '\0', 11);
      printf("\nNew agent's clearance_id: ");
      fgets(clearance_id, 11, stdin);
      check_buffer(clearance_id, 11);

      memset(lang_id, '\0', 11);
      printf("\nNew agent's language_id (enter only one): ");
      fgets(lang_id, 11, stdin);
      check_buffer(lang_id, 11);

      memset(language, '\0', 21);
      printf("\nNew agent's language (enter only one): ");
      fgets(language, 21, stdin);
      check_buffer(language, 21);

      //check if this lang/lang_id pair is ok
      //or if needs added to language table
      lang_check = 0;
      while(!lang_check)
      {
        //set up testing strings
        //is the lang_id in the language table
        memset(string, '\0', 500);
        strcpy(string, "select ");
        strcat(string, lang_id);
        strcat(string, " in(select lang_id from language)");

        res2 = PQexec(conn, string);
        check_tuple(conn, res2);
        char id_in_lang[1] = {'\0'};
        strncpy(id_in_lang, PQgetvalue(res2, 0, 0), 1);
        PQclear(res2);

        //is the language in the language table
        memset(string, '\0', 500);
        strcpy(string, "select '");
        strcat(string, language);
        strcat(string, "' in(select language from language)");

        res2 = PQexec(conn, string);
        check_tuple(conn, res2);
        char lang_in_lang[1] = {'\0'};
        strncpy(lang_in_lang, PQgetvalue(res2, 0, 0), 1);
        PQclear(res2);

        //is the tuple (lang_id, language) in the language table
        memset(string, '\0', 500);
        strcpy(string, "select (");
        strcat(string, lang_id);
        strcat(string, ", '");
        strcat(string, language);
        strcat(string, "') in(select * from language)");

        res2 = PQexec(conn, string);
        check_tuple(conn, res2);
        char id_lang_in_lang[1] = {'\0'};
        strncpy(id_lang_in_lang, PQgetvalue(res2, 0, 0), 1);
        PQclear(res2);

        //if lang_id and lang are not in the table,
        //it is new lang, so add to lang table
        if(id_in_lang[0] == 'f' &&
           lang_in_lang[0] == 'f')
        { 
          lang_check = 1;

          memset(string, '\0', 500);
          strcpy(string, "insert into language values(");
          strcat(string, lang_id);
          strcat(string, ", '");
          strcat(string, language);
          strcat(string, "')");

          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          continue;
        }

        //if (lang_id, language) tuple in language
        //the language exists and no need to add to language table
        if(id_lang_in_lang[0] == 't')
        {
          lang_check = 1;
          continue;
        }

        //any other case is unacceptable
        printf("\nThat language combo is not acceptable.\n");

        //if the language is in the language table
        //show the matching id
        if(lang_in_lang[0] == 't')
        {
          memset(string, '\0', 500);
          strcpy(string, "select lang_id from language where language = '");
          strcat(string, language);
          strcat(string, "'");
          res2 = PQexec(conn, string);
          check_tuple(conn, res2);

          printf("language %s is associated with lang_id ", language);
          printf("%s\n", PQgetvalue(res2, 0, 0));
          PQclear(res2);
        }

        //if the lang_id is in the language table
        //show the matching language
        if(id_in_lang[0] == 't')
        {
          //id not associated with this language
          memset(string, '\0', 500);
          strcpy(string, "select language from language where lang_id = ");
          strcat(string, lang_id);
          res2 = PQexec(conn, string);
          check_tuple(conn, res2);

          printf("lang_id %s is associated with language ", lang_id);
          printf("%s\n", PQgetvalue(res2, 0, 0));
          PQclear(res2);
        }

        //user needs to input an acceptable lang_id/language combo
        //otherwise loop will continue
        memset(lang_id, '\0', 11);
        printf("\nNew agent's language_id (enter only one): ");
        fgets(lang_id, 11, stdin);
        check_buffer(lang_id, 11);

        memset(language, '\0', 21);
        printf("\nNew agent's language (enter only one): ");
        fgets(language, 21, stdin);
        check_buffer(language, 21);
      }//end language check while loop

      //insert row into agent_with_languages
      memset(string, '\0', 500);
      strcpy(string, "insert into agent_with_languages2 values(");
      strcat(string, agent_id);
      strcat(string, ", '");
      strcat(string, first);
      strcat(string, "', '");
      strcat(string, middle);
      strcat(string, "', '");
      strcat(string, last);
      strcat(string, "', '");
      strcat(string, address);
      strcat(string, "', '");
      strcat(string, city);
      strcat(string, "', '");
      strcat(string, country);
      strcat(string, "', ");
      strcat(string, salary);
      strcat(string, ", ");
      strcat(string, clearance_id);
      strcat(string, ", ");
      strcat(string, lang_id);
      strcat(string, ", '");
      strcat(string, language);
      strcat(string, "')");

      res2 = PQexec(conn, string);
      check_command(conn, res2);
      PQclear(res2);

      //since this is a new agent, we need to add to agent table
      memset(string, '\0', 500);
      strcpy(string, "insert into agent values(");
      strcat(string, agent_id);
      strcat(string, ", '");
      strcat(string, first);
      strcat(string, "', '");
      strcat(string, middle);
      strcat(string, "', '");
      strcat(string, last);
      strcat(string, "', '");
      strcat(string, address);
      strcat(string, "', '");
      strcat(string, city);
      strcat(string, "', '");
      strcat(string, country);
      strcat(string, "', ");
      strcat(string, salary);
      strcat(string, ", ");
      strcat(string, clearance_id);
      strcat(string, ")");

      res2 = PQexec(conn, string);
      check_command(conn, res2);
      PQclear(res2);

      //and to the language rel table
      memset(string, '\0', 500);
      strcpy(string, "insert into languagerel values(");
      strcat(string, lang_id);
      strcat(string, ", ");
      strcat(string, agent_id);
      strcat(string, ")");

      res2 = PQexec(conn, string);
      check_command(conn, res2);
      PQclear(res2);
    }//end choice 1

    //choice 2 - delete the current agent
    if(atoi(choice) == 2)
    {
      //delete current agent from agent_with_languages2
      memset(string, '\0', 500);
      strcpy(string, "delete from agent_with_languages2 where agent_id = ");
      strcat(string, PQgetvalue(res, 0, 0));

      res2 = PQexec(conn, string);
      check_command(conn, res2);
      PQclear(res2);

      //delete from agent table (cascades to languagerel table)
      memset(string, '\0', 500);
      strcpy(string, "delete from agent where agent_id = ");
      strcat(string, PQgetvalue(res, 0, 0));

      res2 = PQexec(conn, string);
      check_command(conn, res2);
      PQclear(res2);
      //do not delete from language table since language
      //may be used again

      //move main cursor forward to next agent
      PQclear(res);
      res = PQexec(conn, "fetch next from main");
      check_tuple(conn, res);
      i++;
    }//end if choice = 2

    //choice 3, add language for this agent
    if(atoi(choice) == 3)
    {
      //get the lang_id and language from the user
      memset(lang_id, '\0', 11);
      printf("\nAgent's new language_id (enter only one): ");
      fgets(lang_id, 11, stdin);
      check_buffer(lang_id, 11);

      memset(language, '\0', 21);
      printf("\nAgent's new language (enter only one): ");
      fgets(language, 21, stdin);
      check_buffer(language, 21);

      //check if this lang/lang_id pair is ok
      //or if needs added to language table
      lang_check = 0;
      while(!lang_check)
      {
        //set up testing strings
        //is the lang_id in the language table
        memset(string, '\0', 500);
        strcpy(string, "select ");
        strcat(string, lang_id);
        strcat(string, " in(select lang_id from language)");

        res2 = PQexec(conn, string);
        check_tuple(conn, res2);
        char id_in_lang[1] = {'\0'};
        strncpy(id_in_lang, PQgetvalue(res2, 0, 0), 1);
        PQclear(res2);

        //is the language in the language table
        memset(string, '\0', 500);
        strcpy(string, "select '");
        strcat(string, language);
        strcat(string, "' in(select language from language)");

        res2 = PQexec(conn, string);
        check_tuple(conn, res2);
        char lang_in_lang[1] = {'\0'};
        strncpy(lang_in_lang, PQgetvalue(res2, 0, 0), 1);
        PQclear(res2);

        //is the tuple (lang_id, language) in the language table
        memset(string, '\0', 500);
        strcpy(string, "select (");
        strcat(string, lang_id);
        strcat(string, ", '");
        strcat(string, language);
        strcat(string, "') in(select * from language)");

        res2 = PQexec(conn, string);
        check_tuple(conn, res2);
        char id_lang_in_lang[1] = {'\0'};
        strncpy(id_lang_in_lang, PQgetvalue(res2, 0, 0), 1);
        PQclear(res2);

        //if lang_id and lang are not in the table,
        //it is new lang, so add to lang table
        if(id_in_lang[0] == 'f' &&
           lang_in_lang[0] == 'f')
        { 
          lang_check = 1;

          memset(string, '\0', 500);
          strcpy(string, "insert into language values(");
          strcat(string, lang_id);
          strcat(string, ", '");
          strcat(string, language);
          strcat(string, "')");

          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          continue;
        }

        //if (lang_id, language) tuple in language
        //the language exists and no need to add to language table
        if(id_lang_in_lang[0] == 't')
        {
          lang_check = 1;
          continue;
        }

        //any other case is unacceptable
        printf("\nThat language combo is not acceptable.\n");

        //if the language is in the language table
        //show the matching id
        if(lang_in_lang[0] == 't')
        {
          memset(string, '\0', 500);
          strcpy(string, "select lang_id from language where language = '");
          strcat(string, language);
          strcat(string, "'");
          res2 = PQexec(conn, string);
          check_tuple(conn, res2);

          printf("language %s is associated with lang_id ", language);
          printf("%s\n", PQgetvalue(res2, 0, 0));
          PQclear(res2);
        }

        //if the lang_id is in the language table
        //show the matching language
        if(id_in_lang[0] == 't')
        {
          //id not associated with this language
          memset(string, '\0', 500);
          strcpy(string, "select language from language where lang_id = ");
          strcat(string, lang_id);
          res2 = PQexec(conn, string);
          check_tuple(conn, res2);

          printf("lang_id %s is associated with language ", lang_id);
          printf("%s\n", PQgetvalue(res2, 0, 0));
          PQclear(res2);
        }

        //user needs to input an acceptable lang_id/language combo
        //otherwise loop will continue
        memset(lang_id, '\0', 11);
        printf("\nagent's new language_id (enter only one): ");
        fgets(lang_id, 11, stdin);
        check_buffer(lang_id, 11);

        memset(language, '\0', 21);
        printf("\nagent's new language (enter only one): ");
        fgets(language, 21, stdin);
        check_buffer(language, 21);
      }//end language check while loop

      //make sure that new language is actually new for this agent
      //(does the agent_id, lang_id tuple exist in langrel
      memset(string, '\0', 500);
      strcpy(string, "select (");
      strcat(string, lang_id);
      strcat(string, ", ");
      strcat(string, PQgetvalue(res, 0, agent_id_idx));
      strcat(string, ") in(select * from languagerel)");

      res2 = PQexec(conn, string);
      check_tuple(conn, res2);

      memset(t_f, '\0', 2);
      strcpy(t_f, PQgetvalue(res2, 0, 0));
      PQclear(res2);

      //if the tuple already exists, no need to add
      if(t_f[0] == 't')
        printf("\n\nThat language is already listed, no need to add");
      //otherwise add to agent_with_languages2 and languagerel
      else
        {
          memset(string, '\0', 500);
          strcpy(string, "insert into agent_with_languages2 values(");
          strcat(string, PQgetvalue(res, 0, agent_id_idx));
          strcat(string, ", '");
          strcat(string, PQgetvalue(res, 0, first_idx));
          strcat(string, "', '");
          strcat(string, PQgetvalue(res, 0, middle_idx));
          strcat(string, "', '");
          strcat(string, PQgetvalue(res, 0, last_idx));
          strcat(string, "', '");
          strcat(string, PQgetvalue(res, 0, address_idx));
          strcat(string, "', '");
          strcat(string, PQgetvalue(res, 0, city_idx));
          strcat(string, "', '");
          strcat(string, PQgetvalue(res, 0, country_idx));
          strcat(string, "', ");
          strcat(string, PQgetvalue(res, 0, salary_idx));
          strcat(string, ", ");
          strcat(string, PQgetvalue(res, 0, clearance_id_idx));
          strcat(string, ", ");
          strcat(string, lang_id);
          strcat(string, ", '");
          strcat(string, language);
          strcat(string, "')");

          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          memset(string, '\0', 500);
          strcpy(string, "insert into languagerel values(");
          strcat(string, lang_id);
          strcat(string, ", ");
          strcat(string, PQgetvalue(res, 0, agent_id_idx));
          strcat(string, ")");

          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);
        }
    }//end if choice = 3

    //choice 4 - delete a language for the current agent
    if(atoi(choice) == 4)
    {
      //get the lang_id for the language to delete
      memset(lang_id, '\0', 11);
      printf("\nWhich language do you want to delete\n");
      printf("enter one language_id for the language you want to delete: ");
      fgets(lang_id, 11, stdin);
      check_buffer(lang_id, 11);

      //no need to check that the lang_id is associated for this agent
      //because deleting a non-existing row will not break anything or 
      //cause any errors
      
      //delete from agent_with_languages2 and langrel tables
      //do not delete from language table because the language
      //may be used again by another agent in the future
      memset(string, '\0', 500);
      strcpy(string, "delete from agent_with_languages2 where agent_id = ");
      strcat(string, PQgetvalue(res, 0, agent_id_idx));
      strcat(string, " and lang_id = ");
      strcat(string, lang_id);

      res2 = PQexec(conn, string);
      check_command(conn, res2);
      PQclear(res2);

      memset(string, '\0', 500);
      strcpy(string, "delete from languagerel where agent_id = ");
      strcat(string, PQgetvalue(res, 0, agent_id_idx));
      strcat(string, " and lang_id = ");
      strcat(string, lang_id);

      res2 = PQexec(conn, string);
      check_command(conn, res2);
      PQclear(res2);
    }//end choice 4

    //choice 5 = update attributes of the current agent
    if(atoi(choice) == 5)
    {
      update_done = 0;

      //get the current agent's curr info for updates
      memset(agent_id, '\0', 11);
      strcpy(agent_id, PQgetvalue(res, 0, agent_id_idx));

      memset(first, '\0', 21);
      strcpy(first, PQgetvalue(res, 0, first_idx));

      memset(middle, '\0', 21);
      strcpy(middle, PQgetvalue(res, 0, middle_idx));

      memset(last, '\0', 21);
      strcpy(last, PQgetvalue(res, 0, last_idx));

      memset(address, '\0', 51);
      strcpy(address, PQgetvalue(res, 0, address_idx));

      memset(city, '\0', 21);
      strcpy(city, PQgetvalue(res, 0, city_idx));

      memset(country, '\0', 21);
      strcpy(country, PQgetvalue(res, 0, country_idx));

      memset(salary, '\0', 21);
      strcpy(salary, PQgetvalue(res, 0, salary_idx));

      memset(clearance_id, '\0', 11);
      strcpy(clearance_id, PQgetvalue(res, 0, clearance_id_idx));

      //loop until no more updates
      while(!update_done)
      {
        printf("\n\n1. agent_id    2.first          3.middle       4.last");
        printf("\n5. address     6. city          7. country");
        printf("\n8. salary     9. clearance_id  10. language(id)");
        printf("\n\nSelect from the above list the attribute\n");
        printf("that do you want to update: ");

        memset(choice, '\0', 3);
        fgets(choice, 3, stdin);
        check_buffer(choice, 3);

        //ensure choice is in options
        while(atoi(choice) < 1 || atoi(choice) > 10)
        {
          printf("\nThat's not an option, try again: ");
          memset(choice, '\0', 3);
          fgets(choice, 3, stdin);
          check_buffer(choice, 3);
        }

        if(atoi(choice) == 1)
        {
          printf("\n\nagent's new agent_ID: ");
          memset(agent_id, '\0', 11);
          fgets(agent_id, 11, stdin);
          check_buffer(agent_id, 11);

          //check that the entered agent_id doesn't already exist
          memset(string, '\0', 500);
          strcpy(string, "select ");
          strcat(string, agent_id);
          strcat(string, " in(select agent_id from agent_with_languages2)");

          res2 = PQexec(conn, string);
          check_tuple(conn, res2);

          memset(t_f, '\0', 2);
          strncpy(t_f, PQgetvalue(res2, 0, 0), 1);
          PQclear(res2);

          //if the agent_id entered already exists,
          //show the next highest (after max) and
          if(t_f[0] == 't')
          {
            res2 = PQexec(conn, "select max(agent_id)+1 from agent");
            check_tuple(conn, res2);
            printf("\n\nThat agent_id already exists, try %s\n",
                   PQgetvalue(res2, 0, 0));
            PQclear(res2);
          }
          //otherwise, update agent_with_languages2 and
          //update agetn (languagerel will cascade from agent)
          else
          {
            memset(string, '\0', 500);
            strcpy(string,
             "update agent_with_languages2 set agent_id = ");
            strcat(string, agent_id);
            strcat(string, " where agent_id = ");
            strcat(string, PQgetvalue(res, 0, agent_id_idx));

            res2 = PQexec(conn, string);
            check_command(conn, res2);
            PQclear(res2);
            
            memset(string, '\0', 500);
            strcpy(string, "update agent set agent_id = ");
            strcat(string, agent_id);
            strcat(string, " where agent_id = ");
            strcat(string, PQgetvalue(res, 0, agent_id_idx));

            res2 = PQexec(conn, string);
            check_command(conn, res2);
            PQclear(res2);
          }
        }//end update choice 1

        //choice 2 - first name
        if(atoi(choice) == 2)
        {
          //get updated name
          printf("\n\nAgent's new first name: ");
          memset(first, '\0', 21);
          fgets(first, 21, stdin);
          check_buffer(first, 21);

          //prep sql string for agent_with_languages2
          memset(string, '\0', 500);
          strcpy(string, "update agent_with_languages2 set first = '");
          strcat(string, first);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          //prep sql string for agent 
          memset(string, '\0', 500);
          strcpy(string, "update agent set first = '");
          strcat(string, first);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);
        }//end update choice 2

        //choice 3 - middle name
        if(atoi(choice) == 3)
        {
          //get updated name
          printf("\n\nAgent's new middle name: ");
          memset(middle, '\0', 21);
          fgets(middle, 21, stdin);
          check_buffer(middle, 21);

          //prep sql string for agent_with_languages2
          memset(string, '\0', 500);
          strcpy(string, "update agent_with_languages2 set middle = '");
          strcat(string, middle);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          //prep sql string for agent 
          memset(string, '\0', 500);
          strcpy(string, "update agent set middle = '");
          strcat(string, middle);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);
        }//end update choice 3


        //choice 4 - last name
        if(atoi(choice) == 4)
        {
          //get updated name
          printf("\n\nAgent's new last name: ");
          memset(last, '\0', 21);
          fgets(last, 21, stdin);
          check_buffer(last, 21);

          //prep sql string for agent_with_languages2
          memset(string, '\0', 500);
          strcpy(string, "update agent_with_languages2 set last = '");
          strcat(string, last);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          //prep sql string for agent 
          memset(string, '\0', 500);
          strcpy(string, "update agent set last = '");
          strcat(string, last);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);
        }//end update choice 4

        //choice 5 - last name
        if(atoi(choice) == 5)
        {
          //get updated name
          printf("\n\nAgent's new address: ");
          memset(address, '\0', 51);
          fgets(address, 51, stdin);
          check_buffer(address, 51);

          //prep sql string for agent_with_languages2
          memset(string, '\0', 500);
          strcpy(string, "update agent_with_languages2 set address = '");
          strcat(string, address);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          //prep sql string for agent 
          memset(string, '\0', 500);
          strcpy(string, "update agent set address = '");
          strcat(string, address);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);
        }//end update choice 5

        //choice 6 - city
        if(atoi(choice) == 6)
        {
          //get updated city
          printf("\n\nAgent's new city: ");
          memset(city, '\0', 21);
          fgets(city, 21, stdin);
          check_buffer(city, 21);

          //prep sql string for agent_with_languages2
          memset(string, '\0', 500);
          strcpy(string, "update agent_with_languages2 set city = '");
          strcat(string, city);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          //prep sql string for agent 
          memset(string, '\0', 500);
          strcpy(string, "update agent set city = '");
          strcat(string, city);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);
        }//end update choice 6

        //choice 7 - country
        if(atoi(choice) == 7)
        {
          //get updated name
          printf("\n\nAgent's new country: ");
          memset(country, '\0', 21);
          fgets(country, 21, stdin);
          check_buffer(country, 21);

          //prep sql string for agent_with_languages2
          memset(string, '\0', 500);
          strcpy(string, "update agent_with_languages2 set country = '");
          strcat(string, country);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          //prep sql string for agent 
          memset(string, '\0', 500);
          strcpy(string, "update agent set country = '");
          strcat(string, country);
          strcat(string, "' where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);
        }//end update choice 7

        //choice 8 - salary
        if(atoi(choice) == 8)
        {
          //get updated salary
          printf("\n\nAgent's new salary: ");
          memset(salary, '\0', 21);
          fgets(salary, 21, stdin);
          check_buffer(salary, 21);

          //prep sql string for agent_with_languages2
          memset(string, '\0', 500);
          strcpy(string, "update agent_with_languages2 set salary = ");
          strcat(string, salary);
          strcat(string, " where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          //prep sql string for agent 
          memset(string, '\0', 500);
          strcpy(string, "update agent set salary = ");
          strcat(string, salary);
          strcat(string, " where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);
        }//end update choice 8

        //choice 9 - clearance_id
        if(atoi(choice) == 9)
        {
          //get updated clearance_id
          printf("\n\nAgent's new clearance_id: ");
          memset(clearance_id, '\0', 11);
          fgets(clearance_id, 11, stdin);
          check_buffer(clearance_id, 11);

          //prep sql string for agent_with_languages2
          memset(string, '\0', 500);
          strcpy(string, "update agent_with_languages2 set clearance_id = ");
          strcat(string, clearance_id);
          strcat(string, " where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);

          //prep sql string for agent 
          memset(string, '\0', 500);
          strcpy(string, "update agent set clearance_id = ");
          strcat(string, clearance_id);
          strcat(string, " where agent_id = ");
          strcat(string, agent_id);

          //execute sql command
          res2 = PQexec(conn, string);
          check_command(conn, res2);
          PQclear(res2);
        }//end update choice 9

        //choice 10 - language
        if(atoi(choice) == 10)
        {
          //need to get the language that is beeing updated
          printf("\n\nWhich lang_id is being updated: ");
          memset(lang_id, '\0', 11);
          fgets(lang_id, 11, stdin);
          check_buffer(lang_id, 11);

          //make sure the language is spoken by the current agent
          memset(string, '\0', 500);
          strcpy(string, "select ");
          strcat(string, lang_id);
          strcat(string,
           " in(select lang_id from agent_with_languages2 where agent_id = ");
          strcat(string, agent_id);
          strcat(string, ")");

          res2 = PQexec(conn, string);
          check_tuple(conn, res2);

          memset(t_f, '\0', 2);
          strncpy(t_f, PQgetvalue(res2, 0, 0), 1);
          PQclear(res2);

          //if t_f = false the language isn't spoken by the agent
          //so go back to start
          if(t_f[0] == 'f')
            printf("That language isn't spoken by this agent");
          //otherwise get new lang_id and language and make sure
          //it is a valid pair
          else
          {
            printf("\n\nAgent's new lang_id: ");
            memset(new_lang_id, '\0', 11);
            fgets(new_lang_id, 11, stdin);
            check_buffer(new_lang_id, 11);

            printf("\n\nAgent's new language: ");
            memset(new_language, '\0', 21);
            fgets(new_language, 21, stdin);
            check_buffer(new_language, 21);

            //check if this lang/lang_id pair is ok
            //or if needs added to language table
            lang_check = 0;
            while(!lang_check)
            {
              //set up testing strings
              memset(string, '\0', 500);
              strcpy(string, "select ");
              strcat(string, new_lang_id);
              strcat(string, " in(select lang_id from language)");

              res2 = PQexec(conn, string);
              check_tuple(conn, res2);
              char id_in_lang[1] = {'\0'};
              strncpy(id_in_lang, PQgetvalue(res2, 0, 0), 1);
              PQclear(res2);

              memset(string, '\0', 500);
              strcpy(string, "select '");
              strcat(string, new_language);
              strcat(string, "' in(select language from language)");

              res2 = PQexec(conn, string);
              check_tuple(conn, res2);
              char lang_in_lang[1] = {'\0'};
              strncpy(lang_in_lang, PQgetvalue(res2, 0, 0), 1);
              PQclear(res2);

              memset(string, '\0', 500);
              strcpy(string, "select (");
              strcat(string, new_lang_id);
              strcat(string, ", '");
              strcat(string, new_language);
              strcat(string, "') in(select * from language)");

              res2 = PQexec(conn, string);
              check_tuple(conn, res2);
              char id_lang_in_lang[1] = {'\0'};
              strncpy(id_lang_in_lang, PQgetvalue(res2, 0, 0), 1);
              PQclear(res2);
              
              //if new language need to insert into language table
              if(id_in_lang[0] == 'f' &&
                 lang_in_lang[0] == 'f')
              {
                //new language so just continue and 
                //trigger will insert into language table
                lang_check = 1;

                memset(string, '\0', 500);
                strcpy(string, "insert into language values(");
                strcat(string, new_lang_id);
                strcat(string, ", '");
                strcat(string, new_language);
                strcat(string, "')");

                res2 = PQexec(conn, string);
                check_command(conn, res2);
                PQclear(res2);

                continue;
              }

              //if the combo exists already, no need to add to language table
              if(id_lang_in_lang[0] == 't')
              {
                //language exists already
                lang_check = 1;
                continue;
              }

              //otherwise the combo is unacceptable so get correct combo
              printf("\nThat language combo is not acceptable.\n");

              if(lang_in_lang[0] == 't')
              {
                //language not associated with this id
                memset(string, '\0', 500);
                strcpy(string,
                 "select lang_id from language where language = '");
                strcat(string, new_language);
                strcat(string, "'");
                res2 = PQexec(conn, string);
                check_tuple(conn, res2);

                printf("language %s is associated with lang_id ", new_language);
                printf("%s\n", PQgetvalue(res2, 0, 0));
                PQclear(res2);
              }
              if(id_in_lang[0] == 't')
              {
                //id not associated with this language
                memset(string, '\0', 500);
                strcpy(string,
                   "select language from language where lang_id = ");
                strcat(string, new_lang_id);
                res2 = PQexec(conn, string);
                check_tuple(conn, res2);

                printf("lang_id %s is associated with language ", new_lang_id);
                printf("%s\n", PQgetvalue(res2, 0, 0));
                PQclear(res2);
              }

              //ask again for new id/language
              memset(new_lang_id, '\0', 11);
              printf("\nAgent's new language_id (enter only one): ");
              fgets(new_lang_id, 11, stdin);
              check_buffer(new_lang_id, 11);

              memset(new_language, '\0', 21);
              printf("\nAgent's new language (enter only one): ");
              fgets(new_language, 21, stdin);
              check_buffer(new_language, 21);
            }//end lang_check

            //update in agetn_with_languages2
            //also need to update languagerel table
            memset(string, '\0', 500);
            strcpy(string, "update agent_with_languages2 set language = '");
            strcat(string, new_language);
            strcat(string, "' where agent_id = ");
            strcat(string, agent_id);
            strcat(string, " and lang_id = ");
            strcat(string, lang_id);

            res2 = PQexec(conn, string);
            check_command(conn, res2);
            PQclear(res2);

            memset(string, '\0', 500);
            strcpy(string, "update agent_with_languages2 set lang_id = ");
            strcat(string, new_lang_id);
            strcat(string, " where agent_id = ");
            strcat(string, agent_id);
            strcat(string, " and lang_id = ");
            strcat(string, lang_id);

            res2 = PQexec(conn, string);
            check_command(conn, res2);
            PQclear(res2);
            
            memset(string, '\0', 500);
            strcpy(string, "update languagerel set lang_id = ");
            strcat(string, new_lang_id);
            strcat(string, " where agent_id = ");
            strcat(string, agent_id);
            strcat(string, " and lang_id = ");
            strcat(string, lang_id);

            res2 = PQexec(conn, string);
            check_command(conn, res2);
            PQclear(res2);
          }//end if else (lang spoken by this lang
        }//end update choice 10

        //check if any other updates for this agent
        update_done = other_updates();
        //if done, move to next agent
        if(update_done)
        {
          PQclear(res);
          res = PQexec(conn, "fetch next from main");
          check_tuple(conn, res);
          i++;
        }
      }//end while update not done
    }//end choice = 5

    //choice 6 - nothing needed for this agent, move to next
    if(atoi(choice) == 6)
    {
      PQclear(res);
      res = PQexec(conn, "fetch next from main");
      check_tuple(conn, res);
      i++;
    }

    //choice 7 - done with updates/deletes/inserts, end prog
    if(atoi(choice) == 7)
      break;

  }//end for i rows


  //clean up
  res = PQexec(conn, "close main");
  check_command(conn, res);
  PQclear(res);
  res = PQexec(conn, "end");
  PQfinish(conn);
  return 0;
}//end main


/*
//check if agent table needs updated
      memset(string, '\0', 500);
      strcpy(string, "select ");
      strcat(string, agent_id);
      strcat(string, " in(select agent_id from agent)");

      res2 = PQexec(conn, string);
      check_tuple(conn, res2);

      memset(t_f, '\0', 2);
      strcpy(t_f, PQgetvalue(res2, 0, 0));
      PQclear(res2);

      //if the agent_id is not in the agent
*/
