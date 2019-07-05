# EZ_DRAW 1.2 forked from 

  EZ_DRAW 1.2
  see https://pageperso.lif.univ-mrs.fr/~edouard.thiel/ez-draw/index-en.html
  
  English version there...
  
  contact Edouard.Thiel@lif.univ-mrs.fr
  
  

#Freebasic adaptation, usage for Windows / Linux


Tests Freebasic using static libs for windows  and linux versions 32 and 64 bits.

windows 32 :

    libez-draw_w32.a
    libez-image_w32.a
    
windows 64 :

    libez-draw_w64.a 
    libez-image_w64.a
    

linux 32 :

    libez-draw_l32.a 
    libez-image_l32.a
    
linux 64 :

    libez-draw_l64.a 
    libez-image_l64.a
    

The corresponding 'header' files are

        ez-draw.bi
        ez-image.bi
        
        
One point to remember is the EZ_CALLBACK define (in fact cdecl replacement) to use on the callback function collecting all events
because the static lib functions are cdecl functions  and freebasic use stdcall as default.

ex:

    private sub win1_on_event  EZ_CALLBACK (byval ev as Ez_event ptr)

        select case(ev->type)
            case Expose
                win1_on_expose(ev)
            case KeyPress
                win1_on_key_press(ev)
            case ConfigureNotify
                win1_on_configure_notify(ev)
            case TimerNotify
                win1_on_timer_notify(ev)
         end select
    end sub



usage when creating a window

    a.win1 = ez_window_create(DEF_WIN1_W_ , DEF_WIN1_H_ , "Demo 17: Pixmaps" , procptr( win1_on_event()  ))

