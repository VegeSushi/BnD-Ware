-- Initialize random seed based on when the script loaded
math.randomseed(get_time_ms())

-- Starting variables
local paw_x = 144
local paw_y = 104
local paw_speed = 6
local bubble_x = math.random(20, 320 - 52)
local bubble_y = math.random(20, 240 - 52)

-- Game logic state
local state = "intro"
local state_timer = get_time_ms()
local time_limit = 4000 -- 4 seconds to win

-- Simple collision function
function check_collision(x1, y1, w1, h1, x2, y2, w2, h2)
    return x1 < x2 + w2 and x2 < x1 + w1 and y1 < y2 + h2 and y2 < y1 + h1
end

-- MAIN MICROGAME LOOP
while true do
    begin_frame()
    clear_screen(30, 100, 150) -- Blue water background
    
    local current_time = get_time_ms()

    -- STATE 1: INTRO (Show command for 1 second)
    if state == "intro" then
        draw_text("POP THE BUBBLE!", 100, 110)
        
        if current_time - state_timer > 1000 then
            state = "play"
            state_timer = current_time -- Reset timer for the play phase
        end

    -- STATE 2: PLAY (Move and Pop)
    elseif state == "play" then
        local time_left = time_limit - (current_time - state_timer)
        
        -- Did time run out?
        if time_left <= 0 then
            state = "lose"
            state_timer = current_time
        else
            -- Movement
            if get_button("UP") then paw_y = paw_y - paw_speed end
            if get_button("DOWN") then paw_y = paw_y + paw_speed end
            if get_button("LEFT") then paw_x = paw_x - paw_speed end
            if get_button("RIGHT") then paw_x = paw_x + paw_speed end

            -- Screen boundaries
            if paw_x < 0 then paw_x = 0 end
            if paw_x > 288 then paw_x = 288 end
            if paw_y < 0 then paw_y = 0 end
            if paw_y > 208 then paw_y = 208 end

            -- Popping Logic
            if get_button("A") then
                if check_collision(paw_x, paw_y, 32, 32, bubble_x, bubble_y, 32, 32) then
                    state = "win"
                    state_timer = current_time
                end
            end

            -- Draw entities
            draw_sprite("rom:/sprites/bubble.sprite", bubble_x, bubble_y)
            draw_sprite("rom:/sprites/paw.sprite", paw_x, paw_y)
            
            -- Draw timer text
            draw_text(string.format("Time: %.1f", time_left / 1000.0), 10, 10)
        end

    -- STATE 3: RESULT (Win or Lose)
    elseif state == "win" or state == "lose" then
        -- Draw the final position of the paw
        draw_sprite("rom:/sprites/paw.sprite", paw_x, paw_y)
        
        if state == "win" then
            draw_text("SUCCESS!", 130, 110)
            -- Draw a popped effect? Just text for now.
            draw_text("*POP*", bubble_x, bubble_y) 
        else
            draw_sprite("rom:/sprites/bubble.sprite", bubble_x, bubble_y)
            draw_text("FAILURE!", 130, 110)
        end

        -- Hold the result screen for 1.5 seconds, then exit the microgame
        if current_time - state_timer > 1500 then
            end_frame()
            return -- This EXITS the Lua script and returns control back to C!
        end
    end

    end_frame()
end