//////////////////////////////////////////////////////////////////////////////
//
//  cam4.cpp
//
//  Description:
//      Contains Unigraphics entry points for the application.
//
//////////////////////////////////////////////////////////////////////////////

//  Include files
#include <uf.h>
#include <uf_exit.h>
#include <uf_ui.h>

#include <uf_obj.h>

#include <uf_ui_ont.h>

#include <uf_oper.h>
#include <uf_path.h>

#include <uf_part.h>
#include <uf_param.h>
#include <uf_param_indices.h>

#if ! defined ( __hp9000s800 ) && ! defined ( __sgi ) && ! defined ( __sun )
# include <strstream>
  using std::ostrstream;
  using std::endl;
  using std::ends;
#else
# include <strstream.h>
#endif
#include <iostream.h>

#include "cam6.h"


#define UF_CALL(X) (report( __FILE__, __LINE__, #X, (X)))

static int report( char *file, int line, char *call, int irc)
{
  if (irc)
  {
     char    messg[133];
     printf("%s, line %d:  %s\n", file, line, call);
     (UF_get_fail_message(irc, messg)) ?
       printf("    returned a %d\n", irc) :
       printf("    returned error %d:  %s\n", irc, messg);
  }
  return(irc);
}

int do_cam4_6();

//----------------------------------------------------------------------------
//  Activation Methods
//----------------------------------------------------------------------------

//  Explicit Activation
//      This entry point is used to activate the application explicitly, as in
//      "File->Execute UG/Open->User Function..."
extern "C" DllExport void ufusr( char *parm, int *returnCode, int rlen )
{
    /* Initialize the API environment */
    int errorCode = UF_initialize();

    if ( 0 == errorCode )
    {
        /* TODO: Add your application code here */
        do_cam4_6();

        /* Terminate the API environment */
        errorCode = UF_terminate();
    }

    /* Print out any error messages */
    PrintErrorMessage( errorCode );
}

//----------------------------------------------------------------------------
//  Utilities
//----------------------------------------------------------------------------

// Unload Handler
//     This function specifies when to unload your application from Unigraphics.
//     If your application registers a callback (from a MenuScript item or a
//     User Defined Object for example), this function MUST return
//     "UF_UNLOAD_UG_TERMINATE".
extern "C" int ufusr_ask_unload( void )
{
     /* unload immediately after application exits*/
    return ( UF_UNLOAD_IMMEDIATELY );

     /*via the unload selection dialog... */
     //return ( UF_UNLOAD_SEL_DIALOG );
     /*when UG terminates...              */
     //return ( UF_UNLOAD_UG_TERMINATE );
}

/* PrintErrorMessage
**
**     Prints error messages to standard error and the Unigraphics status
**     line. */
static void PrintErrorMessage( int errorCode )
{
    if ( 0 != errorCode )
    {
        /* Retrieve the associated error message */
        char message[133];
        UF_get_fail_message( errorCode, message );

        /* Print out the message */
        UF_UI_set_status( message );

        fprintf( stderr, "%s\n", message );
    }
}


/*****************************************************************************************/

int do_cam4_6()
{
/*  Variable Declarations */
    char str[133];
    char menu[10][16] ; double  da[10];

    double tool_z_offset;
    int cnt_generate;

    // UF_PATH_id_t path_id; UF_OPER_id_t oper_id;
    // double origin_coordinates[3]; char text[]={""};

    int origin_flag;
    int k;
    UF_UDE_t *ude_objs; int num_of_udes;
    UF_UDE_t ude_object; char *ude_name;
    //int p;  int number_of_params; char **param_names;
    logical resp;

    tag_t       tls = NULL_TAG;
    tag_t       prg = NULL_TAG;
    int i , count = 0 ;
    int   obj_count = 0;
    tag_t *tags = NULL ;
    char  prog_name[UF_OPER_MAX_NAME_LEN+1];
//    int type, subtype ;
    logical generated;
    int generat;
    int response ;
    char *mes1[]={
      "Программа предназначена для установки параметров ....... в операции.",
      "Для этого,Вы должны :",
      "  1) выбрать необходимые операции и нажать кнопку 'Далее..'",
      "  2) в появившемся окне установить необходимые опции ",
      " ",
      NULL
    };
    UF_UI_message_buttons_t buttons1 = { TRUE, FALSE, TRUE, "Далее....", NULL, "Отмена", 1, 0, 2 };
    char *mes2[]={
      "Производить генерацию операции после изменения?",
      NULL
    };
    UF_UI_message_buttons_t buttons2 = { TRUE, FALSE, TRUE, "Генерировать..", NULL, "Нет", 1, 0, 2 };

    response=0;
    //UF_UI_message_dialog("Cam.....", UF_UI_MESSAGE_INFORMATION, mes1, 5, TRUE, &buttons1, &response);
    //if (response!=1) { return (0) ; }

    int module_id;
    UF_ask_application_module(&module_id);
    if (UF_APP_CAM!=module_id) {
       // UF_APP_GATEWAY UF_APP_CAM UF_APP_MODELING UF_APP_NONE
       uc1601("Запуск DLL - производится из модуля обработки\n(ОГТ-ОПУ,КНААПО) - 2005г.",1);
       return (-1);
    }

    /* Ask displayed part tag */
    if (NULL_TAG==UF_PART_ask_display_part()) {
      uc1601("Cam-часть не активна.....\n программа прервана.",1);
      return (0);
    }

  /********************************************************************************/
    // выбранные обьекты и их кол-во
    UF_CALL( UF_UI_ONT_ask_selected_nodes(&obj_count, &tags) );
    if (obj_count<=0) {
      uc1601("Не выбрано операций!\n Программа прервана..",1);
      return(0);
    }

    UF_UI_toggle_stoplight(1);

    if (obj_count) {

    	//UF_UI_open_listing_window();

    	// инициализация массива
    	for(i=0;i<10;i++) { da[i]=0.; }


    	da[0]=200.;
    	da[1]=0.;
    	da[2]=0.;//x
    	da[3]=0.;//y
    	da[4]=0.;//z
      /********************************************************************************/
        //strcpy(&menu[0][0], "Tool Y/Z Offset\0");
        strcpy(&menu[0][0], "Oper Z Offset=\0");
        //strcpy(&menu[1][0], "ORIGIN/x=\0");
        //strcpy(&menu[2][0], "ORIGIN/y=\0");
        strcpy(&menu[1][0], "ORIGIN/z=\0");

        response = uc1609("..Параметры Offset..", menu, 2, da, &i);
        if (response != 3 && response != 4) { return (-1); }

      /********************************************************************************/

        generat=1;
        UF_UI_message_dialog("Cam.....", UF_UI_MESSAGE_QUESTION, mes2, 1, TRUE, &buttons2, &generat);
        if (generat==2) { generat=0; }

      /********************************************************************************/

      for(i=0,count=0;i<obj_count;i++)
      {
         prg = tags[i]; // идентификатор объекта
         prog_name[0]='\0';
         //UF_OBJ_ask_name(prg, prog_name);// спросим имя обьекта
         UF_OPER_ask_name_from_tag(prg, prog_name);

         //UF_UI_write_listing_window("\n"); UF_UI_write_listing_window(prog_name); UF_UI_write_listing_window("\n");

         UF_OPER_ask_cutter_group(prg,&tls);
         if (tls==null_tag) continue;

         cnt_generate=0;

         /* UF_PARAM_TL_Z_OFFSET - UF_PARAM_TL_Z_OFFSET_TOG
          - UF_PARAM_TL_X_OFFSET - UF_PARAM_TL_X_OFFSET_TOG
          - UF_PARAM_TL_Y_OFFSET - UF_PARAM_TL_Y_OFFSET_TOG */
        /*
         UF_PARAM_ask_double_value(tls,UF_PARAM_TL_Z_OFFSET,&tool_z_offset);
         if (tool_z_offset!=da[0]) {
         	 tool_z_offset=da[0];
           UF_PARAM_set_double_value(tls,UF_PARAM_TL_Z_OFFSET,tool_z_offset);
           count++; cnt_generate++;
         }*/

         UF_PARAM_ask_int_value(prg,UF_PARAM_TL_Z_OFFSET_TOG,&response);
         if (response==1) {
             UF_PARAM_ask_double_value(prg,UF_PARAM_TL_Z_OFFSET,&tool_z_offset);
           	 if (tool_z_offset!=da[0]) {
           	   UF_PARAM_set_double_value(prg,UF_PARAM_TL_Z_OFFSET,-9999.);
           	   tool_z_offset=da[0];
           	   UF_PARAM_set_double_value(prg,UF_PARAM_TL_Z_OFFSET,tool_z_offset);
           	   count++; cnt_generate++;
           	 }
            } else {
             UF_PARAM_set_int_value(prg,UF_PARAM_TL_Z_OFFSET_TOG,1);
             UF_PARAM_set_double_value(prg,UF_PARAM_TL_Z_OFFSET,-9999.);
             tool_z_offset=da[0];
             UF_PARAM_set_double_value(prg,UF_PARAM_TL_Z_OFFSET,tool_z_offset);
             count++; cnt_generate++;
          }

         origin_flag=0;
         // есть ли ? - данная функция плохо работает - из-за неизвестности
         // формата данных "origin" или  "Origin" или  "ORIGIN"
         // UF_PARAM_can_accept_ude(prg, UF_UDE_START_SET,"origin",&resp);

         UF_PARAM_ask_udes(prg, UF_UDE_START_SET, &num_of_udes, &ude_objs );
         if (num_of_udes!=0) {

           //str[0]='\0'; sprintf(str,"num_of_udes =%d \n ",num_of_udes); uc1601(str,1);
           for(k=0;k<num_of_udes;k++)
           {
           	ude_object=ude_objs[k];
           	UF_UDE_ask_name(ude_object,&ude_name);  // uc1601(ude_name,1);
           	if (0==strcmp(ude_name,"origin")) {
              /*UF_UDE_ask_params(ude_object,&number_of_params, &param_names );
          	  for(p=0;p<number_of_params;p++)
          	  {
          	  	uc1601(param_names[p],1);
          	  }
          	  UF_free_string_array(number_of_params,param_names);*/
              origin_flag++;
          	  UF_UDE_set_double(ude_object,"X",0.);
          	  UF_UDE_set_double(ude_object,"Y",0.);
          	  UF_UDE_set_double(ude_object,"Z",da[1]);
          	  count++; cnt_generate++;
           	}
           	UF_free(ude_name);
           }
           UF_free(ude_objs);

         }

         if (origin_flag==0) {
           origin_flag++;
           //ude_object=(void *)null_tag;
           // !!!!!!!! Важно !!!!!!!! названия UDE - должны быть маленькими буквами
           UF_PARAM_append_ude(prg,UF_UDE_START_SET,"origin",&ude_object);
           UF_UDE_set_param_toggle(ude_object,"command_status",UF_UDE_PARAM_ACTIVE);
           UF_UDE_set_double(ude_object,"X",0.);
           UF_UDE_set_double(ude_object,"Y",0.);
           UF_UDE_set_double(ude_object,"Z",da[1]);
           UF_UDE_set_string(ude_object,"origin_text","");
           count++; cnt_generate++;
         }

         if (generat==1 && cnt_generate>0) { UF_CALL( UF_PARAM_generate (prg,&generated ) ); }

      }
      UF_free(tags);
    }

    UF_UI_toggle_stoplight(0);

    UF_DISP_refresh ();

    str[0]='\0'; sprintf(str,"Изменено значений =%d \n Просмотрено операций в цикле =%d \n программа завершена.",count,obj_count);
    uc1601(str,1);

 return (0);
}




