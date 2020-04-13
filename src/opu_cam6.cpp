//////////////////////////////////////////////////////////////////////////////
//
//  opu_cam6.cpp
//
//  Description:
//      Contains Unigraphics entry points for the application.
//
//////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_DEPRECATE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

//  Include files
#include <uf.h>
#include <uf_exit.h>
#include <uf_ui.h>

#include <uf_obj.h>

#include <uf_ui_ont.h>

#include <uf_oper.h>
#include <uf_path.h>
#include <uf_ncgroup.h>

#include <uf_part.h>
#include <uf_param.h>
#include <uf_param_indices.h>

#include <uf_defs.h>
#include <uf_styler.h>
#include <uf_mb.h>

/*
#if ! defined ( __hp9000s800 ) && ! defined ( __sgi ) && ! defined ( __sun )
#	include <strstream>
	using std::ostrstream;
	using std::endl;
	using std::ends;
#else
#	include <strstream.h>
#endif
#include <iostream.h>
*/

#include "opu_cam6.h"

//------------------------------------------------------------
#define COUNT_MACHINE 3

struct MACHINES
{
   int num;
   char name[30];
   int oper_z_tog;
   double oper_z;
   int tool_offset_tog[3];
   double tool_offset[3];
   int origin_tog;
   double origin[3];
} ;

// еще одна добавлена - для записи состояния которое задал пользователь
static struct MACHINES machine_list[COUNT_MACHINE+1] =
{
 { 0,"_ФП-14_",0,0.0,{0,0,1},{0.0,0.0,200.0},1,{0.0,0.0,200.0}},
 { 1,"_УгловГолов_",1,25.0,{0,1,1},{0.0,25.0,25.0},1,{25.0,0.0,0.0}},
 { 2,"NONE",0,0.0,{0,0,0},{0.0,0.0,0.0},0,{0.0,0.0,0.0}},
 { 3,"DONE",0,0.0,{0,0,0},{0.0,0.0,0.0},0,{0.0,0.0,0.0}}
};

struct DelParam
{
   int oper_z_tog;
   int tool_offset_tog[3];
   int origin_tog;
   int ude_start_tog;
   int ude_end_tog;
} ;

static struct DelParam Del_Param_list ;

int activated_page = 0 ;

//------------------------------------------------------------

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

/*----------------------------------------------------------------------------*/
#define COUNT_GRP 300

struct GRP
{
   int     num;
   tag_t   tag;
   char    name[UF_OPER_MAX_NAME_LEN+1];
} ;

static struct GRP grp_list[COUNT_GRP] ;
int grp_list_count=0;

/* */
/*----------------------------------------------------------------------------*/
static logical cycleGenerateCb( tag_t   tag, void   *data )
{
   logical  is_group ;
   char     name[UF_OPER_MAX_NAME_LEN + 1];
   int      ecode;

   ecode = UF_NCGROUP_is_group( tag, &is_group );
   //if( is_group != TRUE ) return( TRUE );//!!!!!!!!!!!!!!!!!! важное условие

   name[0]='\0';
   ecode = UF_OBJ_ask_name(tag, name);// спросим имя обьекта
   //UF_UI_write_listing_window("\n");  UF_UI_write_listing_window(name);

   if (grp_list_count>=COUNT_GRP) {
     uc1601("Число выбранных операций/программ\n-превышает лимит в 300 единиц\n обработано только 300 операций",1);
   	 return( FALSE );
   }
   grp_list[grp_list_count].num=grp_list_count;
   grp_list[grp_list_count].tag=tag;
   grp_list[grp_list_count].name[0]='\0';
   sprintf(grp_list[grp_list_count].name,"%s",name);
   grp_list_count++;

   return( TRUE );
}

int loadDll( void );


#include "opu_cam6_dlg.h"


/* The following definition defines the number of callback entries */
/* in the callback structure:                                      */
/* UF_STYLER_callback_info_t PR_cbs */
#define PR_CB_COUNT ( 25 + 1 ) /* Add 1 for the terminator */

/*--------------------------------------------------------------------------
The following structure defines the callback entries used by the
styler file.  This structure MUST be passed into the user function,
UF_STYLER_create_dialog along with PR_CB_COUNT.
--------------------------------------------------------------------------*/
static UF_STYLER_callback_info_t PR_cbs[PR_CB_COUNT] =
{
 {UF_STYLER_DIALOG_INDEX, UF_STYLER_CONSTRUCTOR_CB  , 0, PR_construct_cb},
 {UF_STYLER_DIALOG_INDEX, UF_STYLER_OK_CB           , 0, PR_ok_cb},
 {UF_STYLER_DIALOG_INDEX, UF_STYLER_APPLY_CB        , 0, PR_apply_cb},
 {UF_STYLER_DIALOG_INDEX, UF_STYLER_CANCEL_CB       , 0, PR_cancel_cb},
 {PR_TAB_CTRL_BEG_0     , UF_STYLER_SWITCH_CB       , 0, PR_switch_cb},
 {PR_OPTION_MACH        , UF_STYLER_ACTIVATE_CB     , 0, PR_mach_cb},
 {PR_TOGGLE_OPERZOFFSET , UF_STYLER_VALUE_CHANGED_CB, 0, PR_operzoffset_cb},
 {PR_REAL_OPERZOFFSET   , UF_STYLER_ACTIVATE_CB     , 0, PR_real_cb},
 {PR_TOGGLE_XOFFSET     , UF_STYLER_VALUE_CHANGED_CB, 0, PR_toolxoffset_cb},
 {PR_REAL_TOOLXOFFSET   , UF_STYLER_ACTIVATE_CB     , 0, PR_real_cb},
 {PR_TOGGLE_YZOFFSET    , UF_STYLER_VALUE_CHANGED_CB, 0, PR_toolyzoffset_cb},
 {PR_REAL_TOOLYOFFSET   , UF_STYLER_ACTIVATE_CB     , 0, PR_real_cb},
 {PR_TOGGLE_ZOFFSET     , UF_STYLER_VALUE_CHANGED_CB, 0, PR_toolzoffset_cb},
 {PR_REAL_TOOLZOFFSET   , UF_STYLER_ACTIVATE_CB     , 0, PR_real_cb},
 {PR_TOGGLE_ORIGIN      , UF_STYLER_VALUE_CHANGED_CB, 0, PR_origin_cb},
 {PR_REAL_ORIGINX       , UF_STYLER_ACTIVATE_CB     , 0, PR_real_cb},
 {PR_REAL_ORIGINY       , UF_STYLER_ACTIVATE_CB     , 0, PR_real_cb},
 {PR_REAL_ORIGINZ       , UF_STYLER_ACTIVATE_CB     , 0, PR_real_cb},
 {PR_TOGGLE_DELZOFFSETOPER, UF_STYLER_VALUE_CHANGED_CB, 0, PR_tog_cb},
 {PR_TOGGLE_DELXOFFSETTOOL, UF_STYLER_VALUE_CHANGED_CB, 0, PR_tog_cb},
 {PR_TOGGLE_DELYZOFFSETTOOL, UF_STYLER_VALUE_CHANGED_CB, 0, PR_tog_cb},
 {PR_TOGGLE_DELZOFFSETTOOL, UF_STYLER_VALUE_CHANGED_CB, 0, PR_tog_cb},
 {PR_TOGGLE_DELSTARTORIGIN, UF_STYLER_VALUE_CHANGED_CB, 0, PR_tog_cb},
 {PR_TOGGLE_DELSTARTUDEALL, UF_STYLER_VALUE_CHANGED_CB, 0, PR_tog_cb},
 {PR_TOGGLE_DELENDUDEALL, UF_STYLER_VALUE_CHANGED_CB, 0, PR_tog_cb},
 {UF_STYLER_NULL_OBJECT, UF_STYLER_NO_CB, 0, 0 }
};



/*--------------------------------------------------------------------------
UF_MB_styler_actions_t contains 4 fields.  These are defined as follows:

Field 1 : the name of your dialog that you wish to display.
Field 2 : any client data you wish to pass to your callbacks.
Field 3 : your callback structure.
Field 4 : flag to inform menubar of your dialog location.  This flag MUST
          match the resource set in your dialog!  Do NOT ASSUME that changing
          this field will update the location of your dialog.  Please use the
          UIStyler to indicate the position of your dialog.
--------------------------------------------------------------------------*/
static UF_MB_styler_actions_t actions[] = {
    { "opu_cam6_dlg.dlg",  NULL,   PR_cbs,  UF_MB_STYLER_IS_NOT_TOP },
    { NULL,  NULL,  NULL,  0 } /* This is a NULL terminated list */
};




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
        loadDll();

        /* Terminate the API environment */
        errorCode = UF_terminate();
    }

    /* Print out any error messages */
    PrintErrorMessage( errorCode );
    *returnCode=0;
}


//----------------------------------------------------------------------------
//  Utilities
//----------------------------------------------------------------------------
extern "C" int ufusr_ask_unload (void)
{
     /* unload immediately after application exits*/
     return ( UF_UNLOAD_IMMEDIATELY );

     /*via the unload selection dialog... */
     /*return ( UF_UNLOAD_SEL_DIALOG );   */
     /*when UG terminates...              */
     /*return ( UF_UNLOAD_UG_TERMINATE ); */
}

extern "C" void ufusr_cleanup (void)
{
    return;
}


/*****************************************************************************
**  Activation Methods
*****************************************************************************/

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


int loadDll( void )
{

    int  response   = 0;
    int errorCode;
    char *dialog_name=actions->styler_file;
    char env_names[][25]={
  	"UGII_USER_DIR" ,
  	"UGII_SITE_DIR" ,
  	"UGII_VENDOR_DIR" ,
  	"USER_UFUN" ,
  	"UGII_INITIAL_UFUN_DIR" ,
  	"UGII_TMP_DIR" ,
  	"HOME" ,
  	"TMP" } ;
 int i ;
 char *path , envpath[133] , dlgpath[255] , info[133];
 int status ;

 path = (char *) malloc(133+10);

 errorCode=-1;
  for (i=0;i<7;i++) {

    envpath[0]='\0';
    path=envpath;
    UF_translate_variable(env_names[i], &path);
    if (path!=NULL) {

       /*1*/
       dlgpath[0]='\0';
       strcpy(dlgpath,path); strcat(dlgpath,"\\application\\"); strcat(dlgpath,dialog_name);
       UF_print_syslog(dlgpath,FALSE);

       // работа с файлом
       UF_CFI_ask_file_exist (dlgpath, &status );
       if (!status) { errorCode=0; break ; }

       /*2*/
       dlgpath[0]='\0';
       strcpy(dlgpath,path); strcat(dlgpath,"\\"); strcat(dlgpath,dialog_name);
       UF_print_syslog(dlgpath,FALSE);

       // работа с файлом
       UF_CFI_ask_file_exist (dlgpath, &status );
       if (!status) { errorCode=0; break ; }

     } else { //if (envpath!=NULL)
      info[0]='\0'; sprintf (info,"Переменная %s - не установлена \n ",env_names[i]);
      UF_print_syslog(info,FALSE);
     }
  } // for

 if (errorCode!=0) {
    info[0]='\0'; sprintf (info,"Don't load %s  \n ",dialog_name);
    uc1601 (info, TRUE );
  } else {
       if ( ( errorCode = UF_STYLER_create_dialog ( dlgpath,
               PR_cbs,      /* Callbacks from dialog */
               PR_CB_COUNT, /* number of callbacks*/
           NULL,        /* This is your client data */
           &response ) ) != 0 )
        {
              /* Get the user function fail message based on the fail code.*/
              PrintErrorMessage( errorCode );
         }
  }

 return(errorCode);
}


/*----------------------------------------------------------------------------*/
int _run_change ( tag_t prg , int generat)
{
    char  str[133];
    int count =0 ;
    tag_t       tls = NULL_TAG;
    char  prog_name[UF_OPER_MAX_NAME_LEN+1];
    int type, subtype ;
    int intValue ; double doubleValue ;
    int resp ;

    logical rsp;
    UF_UDE_t *ude_objs; int num_of_udes;
    UF_UDE_t ude_obj; char *ude_name;
    //int p;  int number_of_params; char **param_names;
    int origin_flag, k,prm,prm_tog;
    // UF_PATH_id_t path_id; UF_OPER_id_t oper_id;
    // double origin_coordinates[3]; char text[]={""};

    int cnt_org=0,cnt_tls=0,cnt_opr=0;

    int need_generate;
    logical generated;
//    int generat;

    need_generate=0;

    UF_CALL( UF_OBJ_ask_type_and_subtype (prg, &type, &subtype ) );
    if (type!=UF_machining_operation_type) { return(0) ; }

    prog_name[0]='\0';
    //UF_OBJ_ask_name(prg, prog_name);// спросим имя обьекта
    UF_OPER_ask_name_from_tag(prg, prog_name);
    printf("\n Set prog_name =%s ",prog_name);

    /**********************************/
    UF_OPER_ask_cutter_group(prg,&tls);
    if (tls!=null_tag) {
      for(k=0;k<3;k++)
      {
      	switch (k) {
      		case 0: prm_tog = UF_PARAM_TL_X_OFFSET_TOG ; prm = UF_PARAM_TL_X_OFFSET ;
      		break;
      		case 1: prm_tog = UF_PARAM_TL_Y_OFFSET_TOG ; prm = UF_PARAM_TL_Y_OFFSET ;
      		break;
      		case 2: prm_tog = UF_PARAM_TL_Z_OFFSET_TOG ; prm = UF_PARAM_TL_Z_OFFSET ;
      		break;
      	}

        if (machine_list[COUNT_MACHINE].tool_offset_tog[k]!=0) {
          resp=UF_CALL( UF_PARAM_ask_double_value(tls,prm,&doubleValue) );
          if (machine_list[COUNT_MACHINE].tool_offset[k]!=doubleValue && resp==0) {
            doubleValue=machine_list[COUNT_MACHINE].tool_offset[k];
            UF_CALL( UF_PARAM_set_double_value(tls,prm,-9999.) );
            UF_CALL( UF_PARAM_set_double_value(tls,prm,doubleValue) );
            need_generate++;
          }
        }

      } // for
    } // if

    /**********************************/
    /* UF_PARAM_TL_Z_OFFSET - UF_PARAM_TL_Z_OFFSET_TOG
     - UF_PARAM_TL_X_OFFSET - UF_PARAM_TL_X_OFFSET_TOG
     - UF_PARAM_TL_Y_OFFSET - UF_PARAM_TL_Y_OFFSET_TOG */

    resp = UF_CALL( UF_PARAM_ask_int_value(prg,UF_PARAM_TL_Z_OFFSET_TOG,&intValue) );
    if (machine_list[COUNT_MACHINE].oper_z_tog!=0 && resp==0) {
    	intValue=machine_list[COUNT_MACHINE].oper_z_tog;
      UF_CALL( UF_PARAM_set_int_value(prg,UF_PARAM_TL_Z_OFFSET_TOG,intValue) );
      UF_CALL( UF_PARAM_ask_double_value(prg,UF_PARAM_TL_Z_OFFSET,&doubleValue) );
      if (machine_list[COUNT_MACHINE].oper_z!=doubleValue) {
       doubleValue=machine_list[COUNT_MACHINE].oper_z;
       UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_TL_Z_OFFSET,-9999.) );
       UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_TL_Z_OFFSET,doubleValue) );
      }
      need_generate++;
    }

    /**********************************/
    if (machine_list[COUNT_MACHINE].origin_tog!=0) {

       origin_flag=0;

       /* данный блок работает если ORIGIN(s) - уже существует  */
        UF_CALL( UF_PARAM_ask_udes(prg, UF_UDE_START_SET, &num_of_udes, &ude_objs ) );
        if (num_of_udes!=0) {
         for(k=0;k<num_of_udes;k++)
         {
          ude_obj=ude_objs[k];
          UF_CALL( UF_UDE_ask_name(ude_obj,&ude_name) );
          printf(" %d) ude_name=%s",k,ude_name);
          if (0==strcmp(ude_name,"origin")) {
            /*UF_UDE_ask_params(ude_obj,&number_of_params, &param_names );
            for(p=0;p<number_of_params;p++)
            {
              printf("\t %d) %s",p,param_names[p]);
            }
            UF_free_string_array(number_of_params,param_names);*/
            origin_flag++;
            UF_CALL( UF_UDE_set_double(ude_obj,"X",machine_list[COUNT_MACHINE].origin[0]) );
            UF_CALL( UF_UDE_set_double(ude_obj,"Y",machine_list[COUNT_MACHINE].origin[1]) );
            UF_CALL( UF_UDE_set_double(ude_obj,"Z",machine_list[COUNT_MACHINE].origin[2]) );
            cnt_org++;
            need_generate++;
            UF_free(ude_name); break ; // !!!!!!! Важно меняем токо первое ORIGIN - если оно есть !!!!!!!!
          }
          UF_free(ude_name);
         }
         UF_free(ude_objs);
         num_of_udes=0;
        }

        if (origin_flag==0) {
           //ude_obj=(void *)null_tag;
           // !!!!!!!! Важно !!!!!!!! названия UDE - должны быть маленькими буквами
           UF_CALL(UF_PARAM_can_accept_ude(prg, UF_UDE_START_SET, "origin", &rsp));
           if (rsp) {
             UF_CALL(UF_PARAM_append_ude(prg,UF_UDE_START_SET,"origin",&ude_obj));
             //UF_CALL(UF_UDE_set_param_toggle(ude_obj,"command_status",UF_UDE_PARAM_ACTIVE));
             UF_CALL(UF_UDE_set_double(ude_obj,"X",machine_list[COUNT_MACHINE].origin[0]));
             UF_CALL(UF_UDE_set_double(ude_obj,"Y",machine_list[COUNT_MACHINE].origin[1]));
             UF_CALL(UF_UDE_set_double(ude_obj,"Z",machine_list[COUNT_MACHINE].origin[2]));
             UF_CALL(UF_UDE_set_string(ude_obj,"origin_text",""));
             cnt_org++;
             need_generate++;
             origin_flag++;
           }
        }

        UF_CALL(UF_PARAM_can_accept_ude_set(prg,UF_UDE_START_SET,&rsp));
        if (!rsp) {
          str[0]='\0';
          sprintf(str,"В операции %s \n START User Defined Event - не активировано ",prog_name);
          uc1601(str,1);
        }

        /*
        oper_id = (UF_OPER_id_t) prg;
        UF_CALL( resp=UF_OPER_ask_path(oper_id,&path_id) );
        if (resp==0) {
           UF_CALL( UF_PATH_init_tool_path( path_id ) );
           UF_CALL( UF_PATH_create_origin(path_id,origin_coordinates,text) );
           UF_CALL( UF_PATH_end_tool_path( path_id ) );
        }
        */

    }

 if (generat==1 && need_generate>0) { UF_CALL( UF_PARAM_generate (prg,&generated ) ); }
 count++;
 if (need_generate>0) need_generate=1;

 return(need_generate);
}


/*----------------------------------------------------------------------------*/
int _run_delete ( tag_t prg , int generat)
{
    int count =0 ;
    tag_t       tls = NULL_TAG;
    char  prog_name[UF_OPER_MAX_NAME_LEN+1];
    int type, subtype ;
    int intValue ; double doubleValue ;
    int resp ;

//    logical rsp;
    UF_UDE_t *ude_objs; int num_of_udes;
    UF_UDE_t ude_obj; char *ude_name;
    int k,prm,prm_tog;

    int cnt_org=0,cnt_tls=0,cnt_opr=0;

    int need_generate;
    logical generated;
//    int generat;

    need_generate=0;

    UF_CALL( UF_OBJ_ask_type_and_subtype (prg, &type, &subtype ) );
    if (type!=UF_machining_operation_type) { return(0) ; }

    prog_name[0]='\0';
    //UF_OBJ_ask_name(prg, prog_name);// спросим имя обьекта
    UF_OPER_ask_name_from_tag(prg, prog_name);
    printf("\n Del prog_name =%s ",prog_name);

    /**********************************/
    /* UF_PARAM_TL_Z_OFFSET - UF_PARAM_TL_Z_OFFSET_TOG
     - UF_PARAM_TL_X_OFFSET - UF_PARAM_TL_X_OFFSET_TOG
     - UF_PARAM_TL_Y_OFFSET - UF_PARAM_TL_Y_OFFSET_TOG */

   /*******************************************************/
   if (Del_Param_list.oper_z_tog!=0) {

     UF_CALL( UF_PARAM_ask_int_value(prg,UF_PARAM_TL_Z_OFFSET_TOG,&intValue) );
     if (intValue) {
       UF_CALL( UF_PARAM_ask_double_value(prg,UF_PARAM_TL_Z_OFFSET,&doubleValue) );
       if (doubleValue!=0.) {
         UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_TL_Z_OFFSET,-9999.) );
         UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_TL_Z_OFFSET,0.0) );
       }
       UF_CALL( UF_PARAM_set_int_value(prg,UF_PARAM_TL_Z_OFFSET_TOG,0) );
       cnt_opr++;
       need_generate++;
     }

   }

   /**********************************/
   UF_OPER_ask_cutter_group(prg,&tls);
   if (tls!=null_tag) {
    for(k=0;k<3;k++)
    {
      switch (k) {
      		case 0: prm_tog = UF_PARAM_TL_X_OFFSET_TOG ; prm = UF_PARAM_TL_X_OFFSET ;
      		break;
      		case 1: prm_tog = UF_PARAM_TL_Y_OFFSET_TOG ; prm = UF_PARAM_TL_Y_OFFSET ;
      		break;
      		case 2: prm_tog = UF_PARAM_TL_Z_OFFSET_TOG ; prm = UF_PARAM_TL_Z_OFFSET ;
      		break;
      }
      if (Del_Param_list.tool_offset_tog[k]!=0) {
          resp = 0;
          resp = UF_CALL( UF_PARAM_ask_double_value(tls,prm,&doubleValue) );
          if (doubleValue!=0 && resp==0) {
             UF_CALL( UF_PARAM_set_double_value(tls,prm,-9999.) );
             UF_CALL( UF_PARAM_set_double_value(tls,prm,0.0) );
             need_generate++;
          }
      }

    } // for

    k=1;
    if (Del_Param_list.tool_offset_tog[k]!=0) {
          prm_tog = UF_PARAM_TL_Z_OFFSET_TOG ; prm = UF_PARAM_TL_Z_OFFSET ;
          resp = 0;
          resp = UF_CALL( UF_PARAM_ask_double_value(tls,prm,&doubleValue) );
          if (doubleValue!=0 && resp==0) {
             UF_CALL( UF_PARAM_set_double_value(tls,prm,-9999.) );
             UF_CALL( UF_PARAM_set_double_value(tls,prm,0.0) );
             need_generate++;
          }
    }

   } // if

   /**********************************/
   if (Del_Param_list.origin_tog!=0) {
    UF_CALL(UF_PARAM_ask_udes(prg, UF_UDE_START_SET, &num_of_udes, &ude_objs ));
    if (num_of_udes!=0) {
     printf("\n Del num_of_udes =%d ",num_of_udes);
     for(k=0;k<num_of_udes;k++)
     {
       ude_obj=ude_objs[k];
       UF_CALL(UF_UDE_ask_name(ude_obj,&ude_name));
       if (0==strcmp(ude_name,"origin")) {
          UF_CALL( UF_PARAM_delete_ude(prg, UF_UDE_START_SET, ude_obj) );
          printf("\n\t Del origin =%d (index)",k);
          cnt_org++;
          need_generate++;
       }
       UF_free(ude_name);
     }
     num_of_udes=0;UF_free(ude_objs);
    }
   }

   /**********************************/
   if (Del_Param_list.ude_start_tog!=0) {
    UF_CALL(UF_PARAM_ask_udes(prg, UF_UDE_START_SET, &num_of_udes, &ude_objs ));
    if (num_of_udes!=0) {
    	printf("\n Del UF_UDE_START_SET num_of_udes =%d ",num_of_udes);
    	UF_CALL(UF_PARAM_delete_all_udes(prg, UF_UDE_START_SET));
      num_of_udes=0; UF_free(ude_objs);
    	need_generate++;
    }
   }

   /**********************************/
   if (Del_Param_list.ude_end_tog!=0) {
    UF_CALL(UF_PARAM_ask_udes(prg, UF_UDE_END_SET, &num_of_udes, &ude_objs ));
    if (num_of_udes!=0) {
    	printf("\n Del UF_UDE_END_SET num_of_udes =%d ",num_of_udes);
    	UF_CALL(UF_PARAM_delete_all_udes(prg, UF_UDE_END_SET));
      num_of_udes=0; UF_free(ude_objs);
    	need_generate++;
    }
   }

 if (generat==1 && need_generate>0) { UF_CALL( UF_PARAM_generate (prg,&generated ) ); }
 count++;
 if (need_generate>0) need_generate=1;

 return(need_generate);
}



int _construct_cb ( int dialog_id )
{
    UF_STYLER_item_value_type_t data  ;
    int irc ;
    int i ;
    char *ls[COUNT_MACHINE];

     for (i=0;i<COUNT_MACHINE;i++)
     {
       ls[i]= (char *) malloc ((strlen(machine_list[i].name)+1)*sizeof(char)) ;
       ls[i][0]='\0';
       sprintf(ls[i],"%s",machine_list[i].name);

       UF_print_syslog(ls[i],FALSE);
     }

     data.item_id=PR_OPTION_MACH ;
     data.item_attr=UF_STYLER_SUBITEM_VALUES;
     data.value.strings=ls;
     data.count=COUNT_MACHINE;
     irc=UF_STYLER_set_value(dialog_id,&data);     PrintErrorMessage( irc );

     data.item_attr=UF_STYLER_VALUE ;
     data.subitem_index=0;
     irc=UF_STYLER_set_value(dialog_id,&data);     PrintErrorMessage( irc );

     UF_STYLER_free_value (&data) ;

     free(ls);

     return (0);
}

int _log_tog ( int dialog_id , char *name1 , char *name2)
{
  UF_STYLER_item_value_type_t data  ;
  int irc ;
  int type;

  data.item_attr=UF_STYLER_VALUE;
  data.item_id=name1;
  irc=UF_STYLER_ask_value(dialog_id,&data);    PrintErrorMessage( irc );
  type=data.value.integer;

  data.item_attr=UF_STYLER_SENSITIVITY ;

  data.item_id=name2;
  data.value.integer=type;
  irc=UF_STYLER_set_value(dialog_id,&data);     PrintErrorMessage( irc );

  UF_STYLER_free_value (&data) ;

  return ( 0 );

}

int _list ( int dialog_id )
{
    UF_STYLER_item_value_type_t data  ;
    int irc ;
    int i;

     data.item_id=PR_OPTION_MACH ;
     data.item_attr=UF_STYLER_VALUE;
     irc=UF_STYLER_ask_value(dialog_id,&data);     PrintErrorMessage( irc );
     i=data.value.integer;

     if (i<0) i=0;

  data.item_attr=UF_STYLER_VALUE;

  /*****************************************/

  data.item_id=PR_TOGGLE_OPERZOFFSET;
  data.value.integer=machine_list[i].oper_z_tog;
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

  data.item_id=PR_REAL_OPERZOFFSET;
  data.value.real=machine_list[i].oper_z;
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

     _log_tog (dialog_id , PR_TOGGLE_OPERZOFFSET , PR_REAL_OPERZOFFSET);

  /*****************************************/

  data.item_id=PR_TOGGLE_XOFFSET;
  data.value.integer=machine_list[i].tool_offset_tog[0];
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

  data.item_id=PR_REAL_TOOLXOFFSET;
  data.value.real=machine_list[i].tool_offset[0];
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

     _log_tog (dialog_id , PR_TOGGLE_XOFFSET , PR_REAL_TOOLXOFFSET);

  /*****************************************/

  data.item_id=PR_TOGGLE_YZOFFSET;
  data.value.integer=machine_list[i].tool_offset_tog[2];
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

  data.item_id=PR_REAL_TOOLYOFFSET;
  data.value.real=machine_list[i].tool_offset[1];
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

  data.item_id=PR_REAL_TOOLZOFFSET;
  data.value.real=machine_list[i].tool_offset[2];
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

     _log_tog (dialog_id , PR_TOGGLE_YZOFFSET , PR_REAL_TOOLYOFFSET);
     _log_tog (dialog_id , PR_TOGGLE_YZOFFSET , PR_REAL_TOOLZOFFSET);

  /*****************************************/

  data.item_id=PR_TOGGLE_ORIGIN;
  data.value.integer=machine_list[i].origin_tog;
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

  data.item_id=PR_REAL_ORIGINX;
  data.value.real=machine_list[i].origin[0];
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

  data.item_id=PR_REAL_ORIGINY;
  data.value.real=machine_list[i].origin[1];
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

  data.item_id=PR_REAL_ORIGINZ;
  data.value.real=machine_list[i].origin[2];
  irc=UF_STYLER_set_value(dialog_id,&data);        PrintErrorMessage( irc );

     _log_tog (dialog_id , PR_TOGGLE_ORIGIN , PR_REAL_ORIGINX);
     _log_tog (dialog_id , PR_TOGGLE_ORIGIN , PR_REAL_ORIGINY);
     _log_tog (dialog_id , PR_TOGGLE_ORIGIN , PR_REAL_ORIGINZ);

  /********************************/
  UF_STYLER_free_value (&data) ;

  return (0);
}

int _apply_cb ( int dialog_id )
{
  UF_STYLER_item_value_type_t data  ;
  int irc ;

/*  Variable Declarations */
    char   str[133];
    tag_t  prg = NULL_TAG;
    int    i,j,k , count = 0 ;
    int    obj_count = 0;
    tag_t  *tags = NULL ;
    int    generat;
    int    response ;
    double sum;
//    int    activated_page ;

    char *mes2[]={
      "Производить генерацию операции после изменения?",
      NULL
    };
    UF_UI_message_buttons_t buttons2 = { TRUE, FALSE, TRUE, "Генерировать..", NULL, "Нет", 1, 0, 2 };

    char *mes3[]={
      "Значения массива ORIGIN = 0 (0,0,0) !!!",
      " Создавать для операции UDE оператор ORIGIN/0,0,0",
      "                       ?",
      NULL
    };
    UF_UI_message_buttons_t buttons3 = { TRUE, FALSE, TRUE, "Нет..", NULL, "Создать..", 1, 0, 2 };

  /********************************************************************************/
  data.item_attr=UF_STYLER_VALUE;
  /*****************************************/

  data.item_id=PR_TOGGLE_OPERZOFFSET;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].oper_z_tog=data.value.integer;

  data.item_id=PR_REAL_OPERZOFFSET;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].oper_z=data.value.real;

  /*****************************************/

  data.item_id=PR_TOGGLE_XOFFSET;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].tool_offset_tog[0]=data.value.integer;

  data.item_id=PR_REAL_TOOLXOFFSET;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].tool_offset[0]=data.value.real;

  /*****************************************/

  data.item_id=PR_TOGGLE_YZOFFSET;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].tool_offset_tog[1]=data.value.integer;
  machine_list[COUNT_MACHINE].tool_offset_tog[2]=data.value.integer;

  data.item_id=PR_REAL_TOOLYOFFSET;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].tool_offset[1]=data.value.real;

  data.item_id=PR_REAL_TOOLZOFFSET;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].tool_offset[2]=data.value.real;

  /*****************************************/

  data.item_id=PR_TOGGLE_ORIGIN;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].origin_tog=data.value.integer;

  data.item_id=PR_REAL_ORIGINX;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].origin[0]=data.value.real;

  data.item_id=PR_REAL_ORIGINY;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].origin[1]=data.value.real;

  data.item_id=PR_REAL_ORIGINZ;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  machine_list[COUNT_MACHINE].origin[2]=data.value.real;

  /********************************************************************************/

  data.item_id=PR_TOGGLE_DELZOFFSETOPER;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  Del_Param_list.oper_z_tog=data.value.integer;

  data.item_id=PR_TOGGLE_DELXOFFSETTOOL;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  Del_Param_list.tool_offset_tog[0]=data.value.integer;

  data.item_id=PR_TOGGLE_DELYZOFFSETTOOL;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  Del_Param_list.tool_offset_tog[1]=data.value.integer;

  data.item_id=PR_TOGGLE_DELZOFFSETTOOL;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  Del_Param_list.tool_offset_tog[2]=data.value.integer;

  data.item_id=PR_TOGGLE_DELSTARTORIGIN;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  Del_Param_list.origin_tog=data.value.integer;

  data.item_id=PR_TOGGLE_DELSTARTUDEALL;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  Del_Param_list.ude_start_tog=data.value.integer;

  data.item_id=PR_TOGGLE_DELENDUDEALL;
  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
  Del_Param_list.ude_end_tog=data.value.integer;

  /********************************************************************************/

//  data.item_attr=UF_STYLER_ITEM_TYPE;
//  data.item_id=PR_TAB_CTRL_BEG_0;
//  irc=UF_STYLER_ask_value(dialog_id,&data);        PrintErrorMessage( irc );
//  activated_page=data.value.notify->page_switch.activated_page;

  /********************************************************************************/

  UF_STYLER_free_value (&data) ;
  /********************************************************************************/

  int module_id;
  UF_ask_application_module(&module_id);
  if (UF_APP_CAM!=module_id) {
     uc1601("Запуск DLL - производится из модуля обработки\n...",1);
     return (-1);
  }

  /* Ask displayed part tag */
  if (NULL_TAG==UF_PART_ask_display_part()) {
    uc1601("Cam-часть не активна.....\n ..",1);
    return (0);
  }

  /********************************************************************************/
  // выбранные обьекты и их кол-во
  UF_UI_ONT_ask_selected_nodes(&obj_count, &tags);
  if (obj_count<=0) {
    uc1601("Не выбрано операций/программ!\n ..",1);
    return(0);
  }

  UF_UI_toggle_stoplight(1);

  //UF_UI_open_listing_window();

  /********************************************************************************/
  // генерировать ?
  generat=1;
  UF_UI_message_dialog("Cam.....", UF_UI_MESSAGE_QUESTION, mes2, 1, TRUE, &buttons2, &generat);
  if (generat==2) { generat=0; }

  /********************************************************************************/
  if (activated_page==0) {
    for(k=0,sum=0.;k<3;k++) sum+=fabs(machine_list[COUNT_MACHINE].origin[k]);
    if (sum==0. && machine_list[COUNT_MACHINE].origin_tog!=0) {
      response=1;
      UF_UI_message_dialog("Cam.....", UF_UI_MESSAGE_QUESTION, mes3, 3, TRUE, &buttons3, &response);
      if (response==1) { machine_list[COUNT_MACHINE].origin_tog=0; }
    }
  }
  /********************************************************************************/
  printf("\n\t activated_page =%d   obj_count=%d",activated_page,obj_count);

  for(i=0,count=0;i<obj_count;i++)
  {
    prg = tags[i]; // идентификатор объекта

    grp_list_count=0;// заполняем структуру
    UF_CALL(  UF_NCGROUP_cycle_members( prg, cycleGenerateCb,NULL ) );

   	// эти изменения мы могли проводить в cycleGenerateCb
   	// но я оставил так как было.
    if (grp_list_count==0) {
      if (activated_page==0) count+=_run_change ( prg , generat);
      if (activated_page==1) count+=_run_delete ( prg , generat);
    } else for (j=0;j<grp_list_count;j++) {
   	            if (activated_page==0) count+=_run_change ( grp_list[j].tag , generat);
   	            if (activated_page==1) count+=_run_delete ( grp_list[j].tag , generat);
           }

  } // for
  UF_free(tags);

  UF_UI_ONT_refresh();
  UF_UI_toggle_stoplight(0);

  str[0]='\0'; sprintf(str,"Ready\nObject(s)=%d ",count);
  uc1601(str,1);

  return (0);
}


/*-------------------------------------------------------------------------*/
/*---------------------- UIStyler Callback Functions ----------------------*/
/*-------------------------------------------------------------------------*/

/* -------------------------------------------------------------------------
 * Callback Name: PR_construct_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_construct_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _construct_cb ( dialog_id );
     _list ( dialog_id );

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);
    /* A return value of UF_UI_CB_EXIT_DIALOG will not be accepted    */
    /* for this callback type.  You must continue dialog construction.*/

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_ok_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_ok_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _apply_cb ( dialog_id );

     UF_terminate ();

    /* Callback acknowledged, terminate dialog             */
    /* It is STRONGLY recommended that you exit your       */
    /* callback with UF_UI_CB_EXIT_DIALOG in a ok callback.*/
    /* return ( UF_UI_CB_EXIT_DIALOG );                    */
    return (UF_UI_CB_EXIT_DIALOG);

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_apply_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_apply_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _apply_cb ( dialog_id );

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog                 */
    /* A return value of UF_UI_CB_EXIT_DIALOG will not be accepted    */
    /* for this callback type.  You must respond to your apply button.*/
    return (UF_UI_CB_CONTINUE_DIALOG);

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_cancel_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_cancel_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */

     UF_terminate ();

    /* Callback acknowledged, terminate dialog             */
    /* It is STRONGLY recommended that you exit your       */
    /* callback with UF_UI_CB_EXIT_DIALOG in a cancel call */
    /* back rather than UF_UI_CB_CONTINUE_DIALOG.          */
    return ( UF_UI_CB_EXIT_DIALOG );

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_switch_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_switch_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     activated_page=callback_data->value.notify->page_switch.activated_page;

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}



/* -------------------------------------------------------------------------
 * Callback Name: PR_mach_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_mach_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _list ( dialog_id );

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.    */
    /* return ( UF_UI_CB_EXIT_DIALOG );               */

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_operzoffset_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_operzoffset_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _log_tog (dialog_id , PR_TOGGLE_OPERZOFFSET , PR_REAL_OPERZOFFSET);

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_real_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_real_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.    */
    /* return ( UF_UI_CB_EXIT_DIALOG );               */

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_toolxoffset_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_toolxoffset_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _log_tog (dialog_id , PR_TOGGLE_XOFFSET , PR_REAL_TOOLXOFFSET);

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_toolyzoffset_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_toolyzoffset_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _log_tog (dialog_id , PR_TOGGLE_YZOFFSET , PR_REAL_TOOLYOFFSET);
     _log_tog (dialog_id , PR_TOGGLE_YZOFFSET , PR_REAL_TOOLZOFFSET);

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_toolzoffset_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_toolzoffset_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_origin_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_origin_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _log_tog (dialog_id , PR_TOGGLE_ORIGIN , PR_REAL_ORIGINX);
     _log_tog (dialog_id , PR_TOGGLE_ORIGIN , PR_REAL_ORIGINY);
     _log_tog (dialog_id , PR_TOGGLE_ORIGIN , PR_REAL_ORIGINZ);

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}


/* -------------------------------------------------------------------------
 * Callback Name: PR_tog_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int PR_tog_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}

