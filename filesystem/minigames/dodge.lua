-- dodge.lua - Dodge the falling rocks!
-- Initialize random seed based on when the script loaded
math.randomseed(get_time_ms())

-- Player paw
local paw_x = 144
local paw_y = 200
local paw_speed = 5
local PAW_W = 32
local PAW_H = 32

-- Falling rocks
local ROCK_W = 28
local ROCK_H = 28
local NUM_ROCKS = 3
local rocks = {}

for i = 1, NUM_ROCKS do
    rocks[i] = {
        x = math.random(0, 320 - ROCK_W),
        y = -math.random(0, 200),
        speed = 2 + math.random(0, 3)
    }
end

-- Game logic state
local state = "intro"
local state_timer = get_time_ms()
local survive_time = 4500 -- Survive 4.5 seconds to win
local score = 0

-- Simple collision function
function check_collision(x1, y1, w1, h1, x2, y2, w2, h2)
    return x1 < x2 + w2 and x2 < x1 + w1 and y1 < y2 + h2 and y2 < y1 + h1
end

-- MAIN MICROGAME LOOP
while true do
    begin_frame()
    clear_screen(40, 30, 60) -- Dark cave background

    local current_time = get_time_ms()

    -- STATE 1: INTRO
    if state == "intro" then
        draw_sprite("rom:/sprites/paw.sprite", paw_x, paw_y)
        draw_text("DODGE THE ROCKS!", 90, 60)

        if current_time - state_timer > 1000 then
            state = "play"
            state_timer = current_time
        end

    -- STATE 2: PLAY
    elseif state == "play" then
        local time_left = survive_time - (current_time - state_timer)

        if time_left <= 0 then
            state = "win"
            score = 1000 + math.floor((current_time - state_timer) / 10)
            state_timer = current_time
        else
            -- Movement
            if get_button("UP") then paw_y = paw_y - paw_speed end
            if get_button("DOWN") then paw_y = paw_y + paw_speed end
            if get_button("LEFT") then paw_x = paw_x - paw_speed end
            if get_button("RIGHT") then paw_x = paw_x + paw_speed end

            -- Screen boundaries
            if paw_x < 0 then paw_x = 0 end
            if paw_x > 320 - PAW_W then paw_x = 320 - PAW_W end
            if paw_y < 0 then paw_y = 0 end
            if paw_y > 240 - PAW_H then paw_y = 240 - PAW_H end

            -- Update and draw rocks
            local hit = false
            for i = 1, NUM_ROCKS do
                local r = rocks[i]
                r.y = r.y + r.speed

                if r.y > 240 then
                    r.y = -ROCK_H
                    r.x = math.random(0, 320 - ROCK_W)
                    r.speed = 2 + math.random(0, 3)
                end

                if check_collision(paw_x, paw_y, PAW_W, PAW_H, r.x, r.y, ROCK_W, ROCK_H) then
                    hit = true
                end

                draw_sprite("rom:/sprites/rock.sprite", r.x, r.y)
            end

            draw_sprite("rom:/sprites/paw.sprite", paw_x, paw_y)
            draw_text(string.format("Survive: %.1f", time_left / 1000.0), 10, 10)

            if hit then
                state = "lose"
                score = 0
                state_timer = current_time
            end
        end

    -- STATE 3: RESULT
    elseif state == "win" or state == "lose" then
        draw_sprite("rom:/sprites/paw.sprite", paw_x, paw_y)

        if state == "win" then
            draw_text("SURVIVED!", 120, 110)
        else
            draw_text("SQUASHED!", 115, 110)
        end

        if current_time - state_timer > 1500 then
            end_frame()
            return score
        end
    end

    end_frame()
end
