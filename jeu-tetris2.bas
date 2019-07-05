#include once "ez-draw.bi"

#define WIN_TITLE "Tetrominos        ...  done with EZ-Draw"

#define TIMER1                50                           '50
#define LEVEL_MAX             20                           '20
#define HISTORY_SIZE           4                           '4
#define CHOOSE_TRY             4                           '4


#define GRID_W                10                           '10
#define GRID_H                22                           '22
#define CELLSIZE              30                           '30
#define WIN_WIDTH             ((GRID_W+6)*CELLSIZE)
#define WIN_HEIGHT            ((GRID_H-1)*CELLSIZE)

type e_Type as long
enum
    T_I
    T_O
    T_T
    T_L
    T_G
    T_Z
    T_S
    T_N
end enum

type e_State as long
enum
    S_RUN
    S_PAUSE
    S_LOOSE
    S_LINES
end enum

type e_Boolean as long
enum
    is_false
    is_true
end enum

type t_Tetromino
        typeT                      as e_Type
        i                          as long
        j                          as long
        blocks(0 to 4 , 0 to 4)    as ulong
end type

type t_Controls
        downP                      as e_Boolean
        downR                      as e_Boolean
end type

type t_Game
        next1                      as t_Tetromino
        current                    as t_Tetromino
        history(0 to HISTORY_SIZE - 1) as e_Type
        board(0 to GRID_W - 1 , 0 to GRID_H - 1) as ulong
        level                      as long
        step1                      as long
        score                      as long
        total_lines                as long
        current_lines              as long
        anim_game_over             as long
        anim_lines                 as long
        lines(0 to GRID_H - 1)     as e_Boolean
        state                      as e_State
        prev_state                 as e_State
        controls                   as t_Controls
end type

type t_Data
        win                        as Ez_window
        game                       as t_Game
end type

private sub controls_pressDown(byval that as t_Controls ptr)
    if that->downR then
        that->downR = is_false
        that->downP = is_true
    end if
end sub

private sub controls_releaseDown(byval that as t_Controls ptr)
    that->downR = is_true
    that->downP = is_false
end sub

#define controls_isDownPressed(that) cast(e_Boolean, (that)->downP)

private sub controls_init(byval that as t_Controls ptr)
    that->downP = is_false
    that->downR = is_true
end sub

private sub block_copy(src() as ulong , dst() as ulong)
    dim i                 as long
    dim j                 as long
    i = 0
    while i < 5
        j = 0
        while j < 5
            dst(i , j) = src(i , j)
            j += 1
        wend
        i += 1
    wend
end sub

private sub block_apply_color(src() as ulong , dst() as ulong , byval color1 as ulong)
    dim i                 as long
    dim j                 as long
    i = 0
    while i < 5
        j = 0
        while j < 5
            dst(i , j) = iif(src(i , j) = 1 , color1 , 0)
            j += 1
        wend
        i += 1
    wend
end sub

private sub block_rotate_90(block() as ulong)
    dim tmp(0 to 4, 0 to 4) as ulong
    dim i                 as long
    dim j                 as long
    block_copy(block() , tmp())
    i = 0
    while i < 5
        j = 0
        while j < 5
            block(i , j) = tmp(((5 - 1) - j) , i)
            j += 1
        wend
        i += 1
    wend
end sub

private sub tetromino_rotate(byval that as t_Tetromino ptr)
    block_rotate_90(that->blocks())
end sub

private sub tetromino_top_block(byval that as t_Tetromino ptr)
    dim i                 as long
    dim j                 as long
    j = 0
    while j < 2
        i = 0
        while i < 5
            if that->blocks(i , j) <> 0 then
                that->j += 1
                return
            end if
            i += 1
        wend
        j += 1
    wend
end sub

private sub tetromino_init_Z(byval that as t_Tetromino ptr)
    dim tmp(0 to 4 , 0 to 4) as ulong = {{0 , 0 , 0 , 0 , 0} ,{0 , 1 , 1 , 0 , 0} ,{0 , 0 , 1 , 1 , 0} ,{0 , 0 , 0 , 0 , 0} ,{0 , 0 , 0 , 0 , 0}}
    block_apply_color(tmp() , that->blocks() , ez_red)
end sub

private sub tetromino_init_T(byval that as t_Tetromino ptr)
    dim tmp(0 to 4 , 0 to 4) as ulong = {{0 , 0 , 0 , 0 , 0} ,{0 , 0 , 0 , 0 , 0} ,{0 , 1 , 1 , 1 , 0} ,{0 , 0 , 1 , 0 , 0} ,{0 , 0 , 0 , 0 , 0}}
    block_apply_color(tmp() , that->blocks() , ez_magenta)
end sub

private sub tetromino_init_S(byval that as t_Tetromino ptr)
    dim tmp(0 to 4 , 0 to 4) as ulong = {{0 , 0 , 0 , 0 , 0} ,{0 , 0 , 1 , 1 , 0} ,{0 , 1 , 1 , 0 , 0} ,{0 , 0 , 0 , 0 , 0} ,{0 , 0 , 0 , 0 , 0}}
    block_apply_color(tmp() , that->blocks() , ez_green)
end sub

private sub tetromino_init_O(byval that as t_Tetromino ptr)
    dim tmp(0 to 4 , 0 to 4) as ulong = {{0 , 0 , 0 , 0 , 0} ,{0 , 1 , 1 , 0 , 0} ,{0 , 1 , 1 , 0 , 0} ,{0 , 0 , 0 , 0 , 0} ,{0 , 0 , 0 , 0 , 0}}
    block_apply_color(tmp() , that->blocks() , ez_yellow)
end sub

private sub tetromino_init_L(byval that as t_Tetromino ptr)
    dim color1 as ulong = ez_get_RGB(237 , 127 , 16)
    dim tmp(0 to 4 , 0 to 4) as ulong = {{0 , 0 , 0 , 0 , 0} ,{0 , 0 , 0 , 0 , 0} ,{0 , 1 , 1 , 1 , 0} ,{0 , 1 , 0 , 0 , 0} ,{0 , 0 , 0 , 0 , 0}}
    block_apply_color(tmp() , that->blocks() , color1)
end sub

private sub tetromino_init_I(byval that as t_Tetromino ptr)
    dim tmp(0 to 4 , 0 to 4) as ulong = {{0 , 0 , 0 , 0 , 0} ,{0 , 0 , 0 , 0 , 0} ,{0 , 1 , 1 , 1 , 1} ,{0 , 0 , 0 , 0 , 0} ,{0 , 0 , 0 , 0 , 0}}
    block_apply_color(tmp() , that->blocks() , ez_cyan)
end sub

private sub tetromino_init_G(byval that as t_Tetromino ptr)
    dim tmp(0 to 4 , 0 to 4) as ulong = {{0 , 0 , 0 , 0 , 0} ,{0 , 0 , 0 , 0 , 0} ,{0 , 1 , 1 , 1 , 0} ,{0 , 0 , 0 , 1 , 0} ,{0 , 0 , 0 , 0 , 0}}
    block_apply_color(tmp() , that->blocks() , ez_blue)
end sub

private sub tetromino_init_blocks(byval that as t_Tetromino ptr)
    if that->typeT = T_Z then
        tetromino_init_Z(that)
    elseif that->typeT = T_T then
        tetromino_init_T(that)
    elseif that->typeT = T_S then
        tetromino_init_S(that)
    elseif that->typeT = T_O then
        tetromino_init_O(that)
    elseif that->typeT = T_L then
        tetromino_init_L(that)
    elseif that->typeT = T_I then
        tetromino_init_I(that)
    elseif that->typeT = T_G then
        tetromino_init_G(that)
    end if
    tetromino_rotate(that)
    tetromino_top_block(that)
end sub

private sub tetromino_init(byval that as t_Tetromino ptr , byval t as e_Type)
    that->typeT = t
    that->i = GRID_W / 4
    that->j = - 2
    tetromino_init_blocks(that)
end sub

private sub game_init_history(byval that as t_Game ptr)
    dim i                 as long
    i = 0
    while i < HISTORY_SIZE
        that->history(i) = iif(ez_random(2) , T_S , T_Z)
        i += 1
    wend
end sub

private function game_is_in_history(byval that as t_Game ptr , byval t as e_Type) as e_Boolean
    dim i                 as long
    i = 0
    while i < HISTORY_SIZE
        if that->history(i) = t then
            return is_true
        end if
        i += 1
    wend
    return is_false
end function

private sub game_choose_next1(byval that as t_Game ptr)
    dim i                 as long
    dim t                 as e_Type
    i = 0
    while i < CHOOSE_TRY
        t = ez_random(T_N)
        if game_is_in_history(that , t) = 0 then
            exit while
        end if
        i += 1
    wend

    tetromino_init(@that->next1 , t)
    i = HISTORY_SIZE - 1
    while i > 0
        that->history(i) = that->history((i - 1))
        i -= 1
    wend
    that->history(0) = t
end sub

private sub game_init_board(byval that as t_Game ptr)
    dim i                 as long
    dim j                 as long
    i = 0
    while i < GRID_W
        j = 0
        while j < GRID_H
            that->board(i , j) = 0
            j += 1
        wend
        i += 1
    wend
end sub

private function game_current_can_move(byval that as t_Game ptr , byval x as long , byval y as long) as e_Boolean
    dim c as t_Tetromino ptr = @that->current
    dim i                 as long
    dim j                 as long
    i = 0
    while i < 5
        j = 0
        while j < 5
            if c->blocks(i , j) = 0 then
                goto continue1
            end if
            if (((c->i + i) + x) >= GRID_W) orelse(((c->i + i) + x) < 0) then
                return is_false
            end if
            if ((c->j + j) + y) >= (GRID_H - 1) then
                return is_false
            end if
            if ((c->j + j) + y) < 0 then
                goto continue1
            end if
            if that->board(((c->i + i) + x) ,((c->j + j) + y)) <> 0 then
                return is_false
            end if
continue1:
            j += 1
        wend
        i += 1
    wend
    return is_true
end function

private sub game_take_next1(byval that as t_Game ptr)
    that->current = that->next1
    game_choose_next1(that)
    if game_current_can_move(that , 0 , 0) = 0 then
        that->state = S_LOOSE
    end if
end sub

private function game_is_level_up(byval that as t_Game ptr) as e_Boolean
    return - ((that->total_lines > (3 + (that->level * that->level))) andalso(that->level < LEVEL_MAX))
end function

private sub game_reset_lines(byval that as t_Game ptr)
    dim i                 as long
    i = 0
    while i < GRID_H
        that->lines(i) = is_false
        i += 1
    wend
end sub

private function game_is_line_full(byval that as t_Game ptr , byval i as long) as e_Boolean
    dim j                 as long
    j = 0
    while j < GRID_W
        if that->board(j , i) = 0 then
            return is_false
        end if
        j += 1
    wend
    return is_true
end function

private sub game_check_lines(byval that as t_Game ptr)
    dim i                 as long
    dim n as long = 0
    game_reset_lines(that)
    i = 0
    while i < GRID_H
        if game_is_line_full(that , i) then
            that->lines(i) = is_true
            n += 1
        end if
        i += 1
    wend
    that->current_lines = n
    if n > 0 then
        that->state = S_LINES
    end if
end sub

private sub game_current_put(byval that as t_Game ptr)
    dim i                 as long
    dim j                 as long
    dim c as t_Tetromino ptr = @that->current
    i = 0
    while i < 5
        j = 0
        while j < 5
            if c->blocks(i , j) <> 0 then
                that->board((c->i + i) ,(c->j + j)) = c->blocks(i , j)
            end if
            j += 1
        wend
        i += 1
    wend
    game_check_lines(that)
    if game_is_level_up(that) then
        that->level += 1
    end if
    game_take_next1(that)
    that->controls.downP = is_false
end sub

private sub game_shift_board(byval that as t_Game ptr , byval k as long , byval n as long)
    dim i                 as long
    dim j                 as long
    i = k - 1
    while i >= 0
        j = 0
        while j < GRID_W
            that->board(j ,(i + n)) = that->board(j , i)
            j += 1
        wend
        i -= 1
    wend
    j = 0
    while j < GRID_W
        that->board(j , 0) = 0
        j += 1
    wend
end sub

private sub game_add_lines_score(byval that as t_Game ptr)
    if that->current_lines = 1 then
        that->score += that->level * 10
    elseif that->current_lines = 2 then
        that->score += that->level * 100
    elseif that->current_lines = 3 then
        that->score += that->level * 500
    elseif that->current_lines = 4 then
        that->score += that->level * 2000
    end if
end sub

private sub game_do_lines(byval that as t_Game ptr)
    dim i                 as long
    game_add_lines_score(that)
    that->total_lines += that->current_lines
    i = 0
    while i < GRID_H
        if that->lines(i) then
            game_shift_board(that , i , 1)
        end if
        i += 1
    wend
    that->anim_lines = 0
    that->state = S_RUN
end sub

private function game_current_fall(byval that as t_Game ptr) as e_Boolean
    if game_current_can_move(that , 0 , 1) then
        that->current.j += 1
        if that->current.j > 3 then
            that->score += that->level
        end if
        return is_true
    else
        game_current_put(that)
        return is_false
    end if
end function

private sub game_current_auto_fall(byval that as t_Game ptr)
    that->step1 = (that->step1 + 1) mod((LEVEL_MAX + 1) - that->level)
    if controls_isDownPressed(@that->controls) then
        game_current_fall(that)
    elseif that->step1 = 0 then
        game_current_fall(that)
    end if
end sub

private sub game_current_full_fall(byval that as t_Game ptr)
    while game_current_fall(that)
    wend
end sub

private function game_is_line_clipping(byval that as t_Game ptr , byval j as long) as e_Boolean
    return - (((that->state = S_LINES) andalso that->lines(j)) andalso(that->anim_lines mod 2))
end function

private sub game_current_move(byval that as t_Game ptr , byval x as long)
    if game_current_can_move(that , x , 0) then
        that->current.i += x
    end if
end sub

private sub game_current_rotate(byval that as t_Game ptr)
    dim i                 as long
    if that->current.typeT = T_O then
        return
    end if
    tetromino_rotate(@that->current)
    if game_current_can_move(that , 0 , 0) then
        return
    end if
    i = 1
    while i < 5
        if game_current_can_move(that , i , 0) then
            that->current.i += i
            return
        elseif game_current_can_move(that , - i , 0) then
            that->current.i -= i
            return
        elseif game_current_can_move(that , 0 , - i) then
            that->current.j -= i
            return
        end if
        i += 1
    wend
    i = 0
    while i < 3
        tetromino_rotate(@that->current)
        i += 1
    wend
end sub

private sub game_put_grey_line(byval that as t_Game ptr)
    dim i                 as long
    dim j                 as long
    j = (GRID_H - that->anim_game_over) - 1
    i = 0
    while i < GRID_W
        that->board(i , j) = ez_grey
        i += 1
    wend
end sub

private sub game_anim_game_over(byval that as t_Game ptr)
    if that->anim_game_over <= GRID_H then
        game_put_grey_line(that)
        that->anim_game_over += 1
    end if
end sub

private sub game_anim_lines(byval that as t_Game ptr)
    that->anim_lines += 1
    if that->anim_lines >= 10 then
        game_do_lines(that)
    end if
end sub

private sub game_toogle_pause(byval that as t_Game ptr)
    dim s                 as e_State
    if that->state = S_LOOSE then
        return
    end if
    s = that->prev_state
    that->prev_state = that->state
    that->state = iif(that->state <> S_PAUSE , S_PAUSE , s)
end sub

private sub game_init(byval that as t_Game ptr)
    game_init_history(that)
    game_choose_next1(that)
    game_init_board(that)
    game_take_next1(that)
    that->step1 = 0
    that->level = 1
    that->score = 0
    that->total_lines = 0
    that->state = S_RUN
    that->current_lines = 0
    that->anim_game_over = 0
    that->anim_lines = 0
    controls_init(@that->controls)
end sub

private sub cell_draw(byval win as Ez_window , byval x as long , byval y as long , byval color1 as ulong)
    ez_set_color(color1)
    ez_fill_rectangle(win , x , y , x + CELLSIZE , y + CELLSIZE)
    ez_set_thick(2)
    ez_set_color(ez_grey)
    ez_draw_rectangle(win , x + 1 , y + 1 ,(x + CELLSIZE) - 1 ,(y + CELLSIZE) - 1)
    ez_set_thick(3)
    ez_set_color(ez_black)
    ez_draw_rectangle(win , x , y , x + CELLSIZE , y + CELLSIZE)
end sub

private sub shadow_draw(byval win as Ez_window , byval i as long , byval j as long)
    dim x as long = i * CELLSIZE
    dim y as long = j * CELLSIZE
    ez_set_color(ez_grey)
    ez_fill_rectangle(win , x , y , x + CELLSIZE , y + CELLSIZE)
end sub

private sub game_cell_draw(byval that as t_Game ptr , byval win as Ez_window , byval i as long , byval j as long)
    dim x as long = i * CELLSIZE
    dim y as long = j * CELLSIZE
    cell_draw(win , x , y , that->board(i , j))
end sub

private sub tetromino_draw(byval tetro as t_Tetromino ptr , byval win as Ez_window , byval x as long , byval y as long)
    dim i                 as long
    dim j                 as long
    i = 0
    while i < 5
        j = 0
        while j < 5
            if tetro->blocks(i , j) <> 0 then
                cell_draw(win , x + (i * CELLSIZE) , y + (j * CELLSIZE) , tetro->blocks(i , j))
            end if
            j += 1
        wend
        i += 1
    wend
end sub

private sub game_current_draw(byval that as t_Game ptr , byval win as Ez_window)
    dim i                 as long
    dim j                 as long
    dim y                 as long
    dim c as t_Tetromino ptr = @that->current
    if (that->state = S_LINES) orelse(that->prev_state = S_LINES) then
        return
    end if
    y = 1
    while game_current_can_move(that , 0 , y)
        y += 1
    wend
    i = 0
    while i < 5
        j = 0
        while j < 5
            if c->blocks(i , j) <> 0 then
                shadow_draw(win , c->i + i ,((c->j + j) + y) - 1)
            end if
            j += 1
        wend
        i += 1
    wend
    tetromino_draw(c , win , c->i * CELLSIZE , c->j * CELLSIZE)
end sub

private sub game_board_draw(byval that as t_Game ptr , byval win as Ez_window)
    dim i                 as long
    dim j                 as long
    i = 0
    while i < GRID_W
        j = 0
        while j < GRID_H
            if game_is_line_clipping(that , j) then
                goto continue2
            end if
            if that->board(i , j) <> 0 then
                game_cell_draw(that , win , i , j)
            end if
continue2:
            j += 1
        wend
        i += 1
    wend
end sub

private sub game_Controls_draw(byval that as t_Game ptr , byval win as Ez_window , byval x as long , byval y as long)
    ez_set_color(ez_white)
    ez_set_nfont(2)
    ez_draw_text(win , EZ_ML , x , y , "CONTROLS")
    ez_set_nfont(1)
    ez_draw_text(win , EZ_ML , x , y + CELLSIZE , "<- ->: Move")
    ez_draw_text(win , EZ_ML , x , y + (CELLSIZE * 2) , "Down:  Fast fall")
    ez_draw_text(win , EZ_ML , x , y + (CELLSIZE * 3) , "Up:    Full fall")
    ez_draw_text(win , EZ_ML , x , y + (CELLSIZE * 4) , "Space: Rotate")
    ez_draw_text(win , EZ_ML , x , y + (CELLSIZE * 5) , "P:     Pause")
end sub

private sub game_ui_draw(byval that as t_Game ptr , byval win as Ez_window)
    dim i                 as long
    dim xtip as long = GRID_W * CELLSIZE
    dim ytip as long = 0
    ez_set_color(ez_black)
    ez_fill_rectangle(win , xtip , ytip , WIN_WIDTH , WIN_HEIGHT)
    ez_set_nfont(3)
    ez_set_color(ez_white)
    ez_draw_text(win , EZ_ML , xtip + CELLSIZE , ytip + CELLSIZE , "SCORE")
    ez_draw_text(win , EZ_ML , xtip + CELLSIZE , ytip + (CELLSIZE * 2) , "%010d" , that->score)
    ez_draw_text(win , EZ_ML , xtip + CELLSIZE , ytip + (CELLSIZE * 4) , "LEVEL")
    ez_draw_text(win , EZ_ML , xtip + CELLSIZE , ytip + (CELLSIZE * 5) , "%02d" , that->level)
    ez_draw_text(win , EZ_ML , xtip + CELLSIZE , ytip + (CELLSIZE * 7) , "LINES")
    ez_draw_text(win , EZ_ML , xtip + CELLSIZE , ytip + (CELLSIZE * 8) , "%04d" , that->total_lines)
    ez_draw_text(win , EZ_ML , xtip + CELLSIZE , ytip + (CELLSIZE * 10) , "Next")
    tetromino_draw(@that->next1 , win , xtip + 1 , ytip + (CELLSIZE * 10))
    ez_set_thick(1)
    ez_set_color(ez_grey)
    i = 0
    while i <= GRID_W
        ez_draw_line(win , i * CELLSIZE , 0 , i * CELLSIZE ,(GRID_H - 1) *CELLSIZE)
        i += 1
    wend
    game_Controls_draw(that , win , xtip + CELLSIZE , ytip + (CELLSIZE * 15))
end sub

private sub game_display_gameover(byval that as t_Game ptr , byval win as Ez_window)
    if ((that->state = S_LOOSE) andalso(that->anim_game_over > 20)) = 0 then
        return
    end if
    ez_set_color(ez_white)
    ez_fill_rectangle(win , 0 ,((GRID_H - 4) / 2) *CELLSIZE , GRID_W * CELLSIZE ,((GRID_H + 2) / 2) *CELLSIZE)
    ez_set_color(ez_red)
    ez_set_nfont(3)
    ez_draw_text(win , EZ_MC ,(GRID_W / 2) *CELLSIZE ,((GRID_H - 1) / 2) *CELLSIZE , "GAME OVER")
    ez_set_nfont(2)
    ez_draw_text(win , EZ_MC ,(GRID_W / 2) *CELLSIZE ,((GRID_H + 1) / 2) *CELLSIZE , "PRESS SPACE TO RESTART")
end sub

private sub game_display_pause(byval that as t_Game ptr , byval win as Ez_window)
    if that->state <> S_PAUSE then
        return
    end if
    ez_set_color(ez_white)
    ez_fill_rectangle(win , 0 ,((GRID_H - 4) / 2) *CELLSIZE , GRID_W * CELLSIZE ,((GRID_H + 2) / 2) *CELLSIZE)
    ez_set_color(ez_red)
    ez_set_nfont(3)
    ez_draw_text(win , EZ_MC ,(GRID_W / 2) *CELLSIZE ,((GRID_H - 1) / 2) *CELLSIZE , "PAUSE")
    ez_set_nfont(2)
    ez_draw_text(win , EZ_MC ,(GRID_W / 2) *CELLSIZE ,((GRID_H + 1) / 2) *CELLSIZE , "PRESS P TO RESTART")
end sub

private sub win_on_expose(byval ev as Ez_event ptr)
    dim data1 as t_Data ptr = ez_get_data(ev->win)
    dim game as t_Game ptr = @data1->game
    game_ui_draw(game , data1->win)
    game_current_draw(game , data1->win)
    game_board_draw(game , data1->win)
    game_display_gameover(game , data1->win)
    game_display_pause(game , data1->win)
end sub

private sub win_on_timerNotify(byval ev as Ez_event ptr)
    dim data1 as t_Data ptr = ez_get_data(ev->win)
    dim game as t_Game ptr = @data1->game
    if game->state = S_RUN then
        game_current_auto_fall(game)
    elseif game->state = S_LINES then
        game_anim_lines(game)
    elseif game->state = S_LOOSE then
        game_anim_game_over(game)
    end if
    ez_start_timer(data1->win , TIMER1)
    ez_send_expose(data1->win)
end sub

private sub win_on_keyPress_run(byval ev as Ez_event ptr)
    dim data1 as t_Data ptr = ez_get_data(ev->win)
    dim game as t_Game ptr = @data1->game
    if ev->key_sym = XK_Left then
        game_current_move(game , - 1)
    elseif ev->key_sym = XK_Right then
        game_current_move(game , 1)
    elseif ev->key_sym = XK_space then
        game_current_rotate(game)
    elseif ev->key_sym = XK_Down then
        controls_pressDown(@game->controls)
    elseif ev->key_sym = XK_Up then
        game_current_full_fall(game)
    end if
end sub

private sub win_on_keyPress_loose(byval ev as Ez_event ptr)
    dim data1 as t_Data ptr = ez_get_data(ev->win)
    dim game as t_Game ptr = @data1->game
    if game->anim_game_over < GRID_H then
        return
    end if
    if ev->key_sym = XK_space then
        game_init(game)
    end if
end sub

private sub win_on_keyPress(byval ev as Ez_event ptr)
    dim data1 as t_Data ptr = ez_get_data(ev->win)
    dim game as t_Game ptr = @data1->game
    if ev->key_sym = XK_p then
        game_toogle_pause(game)
    end if
    if game->state = S_RUN then
        win_on_keyPress_run(ev)
    elseif game->state = S_LINES then
    elseif game->state = S_LOOSE then
        win_on_keyPress_loose(ev)
    end if
end sub

private sub win_on_keyRelease(byval ev as Ez_event ptr)
    dim data1 as t_Data ptr = ez_get_data(ev->win)
    dim game as t_Game ptr = @data1->game
    if ev->key_sym = XK_Down then
        controls_releaseDown(@game->controls)
    end if
end sub

private sub win_on_event EZ_CALLBACK(byval ev as Ez_event ptr)
    if ev->type = Expose then
        win_on_expose(ev)
    elseif ev->type = KeyPress then
        win_on_keyPress(ev)
    elseif ev->type = KeyRelease then
        win_on_keyRelease(ev)
    elseif ev->type = TimerNotify then
        win_on_timerNotify(ev)
    end if
end sub

private sub data_init(byval data1 as t_Data ptr)
    data1->win = ez_window_create(WIN_WIDTH , WIN_HEIGHT , WIN_TITLE , procptr(win_on_event))
    game_init(@data1->game)
    ez_window_dbuf(data1->win , is_true)
    ez_set_data(data1->win , data1)
    ez_start_timer(data1->win , TIMER1)
end sub

private sub Sub_exit(byval i as long)
    if i <> 0 then
        printf "Error , any key to close"
    else
        printf "Any key to close"
    END IF
    'sleep

    end(abs(i))
end sub

private sub sub_main()
    dim data1             as t_Data
    if ez_init() < 0 then sub_exit(1)

    data_init(@data1)
    ez_main_loop()
    sub_exit(0)
end sub


sub_main()

