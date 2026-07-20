-- feed.lua - Catch the falling sushi in Bubbles' mouth!
-- Initialize random seed based on when the script loaded
math.randomseed(get_time_ms())

-- Bubbles sprite is 64x54, sits along the bottom and slides left/right
local BUBBLES_W = 64
local BUBBLES_H = 54
local bubbles_x = (320 - BUBBLES_W) / 2
local bubbles_y = 240 - BUBBLES_H - 6
local bubbles_speed = 5

-- Falling sushi
local SUSHI_W = 32
local SUSHI_H = 32
local sushi_x = math.random(10, 320 - SUSHI_W - 10)
local sushi_y = -SUSHI_H
local sushi_speed = 3

-- Game logic state
local state = "intro"
local state_timer = get_time_ms()
local score = 0

-- Simple collision function
function check_collision(x1, y1, w1, h1, x2, y2, w2, h2)
    return x1 < x2 + w2 and x2 < x1 + w1 and y1 < y2 + h2 and y2 < y1 + h1
end

-- MAIN MICROGAME LOOP
while true do
    begin_frame()
    clear_screen(20, 140, 200) -- Bright ocean-blue background

    local current_time = get_time_ms()

    -- STATE 1: INTRO
    if state == "intro" then
        draw_sprite("rom:/sprites/bubbles.sprite", (320 - BUBBLES_W) / 2, 90)
        draw_text("FEED BUBBLES!", 100, 60)

        if current_time - state_timer > 1000 then
            state = "play"
            state_timer = current_time
        end

    -- STATE 2: PLAY
    elseif state == "play" then
        -- Move Bubbles left/right along the bottom
        if get_button("LEFT") then bubbles_x = bubbles_x - bubbles_speed end
        if get_button("RIGHT") then bubbles_x = bubbles_x + bubbles_speed end

        if bubbles_x < 0 then bubbles_x = 0 end
        if bubbles_x > 320 - BUBBLES_W then bubbles_x = 320 - BUBBLES_W end

        -- Sushi falls
        sushi_y = sushi_y + sushi_speed

        -- Check for a catch every frame while the sushi overlaps mouth level
        if check_collision(bubbles_x, bubbles_y, BUBBLES_W, BUBBLES_H,
                            sushi_x, sushi_y, SUSHI_W, SUSHI_H) then
            state = "win"
            -- Reward precision: closer to the mouth's center = higher score
            local bubbles_center = bubbles_x + (BUBBLES_W / 2)
            local sushi_center = sushi_x + (SUSHI_W / 2)
            local dist = math.abs(bubbles_center - sushi_center)
            score = 1000 + math.max(0, 200 - math.floor(dist * 4))
            state_timer = current_time
        -- Only a loss once the sushi has fully fallen past the paddle
        elseif sushi_y > bubbles_y + BUBBLES_H then
            state = "lose"
            score = 0
            state_timer = current_time
        else
            -- Draw entities
            draw_sprite("rom:/sprites/sushi.sprite", sushi_x, sushi_y)
            draw_sprite("rom:/sprites/bubbles.sprite", bubbles_x, bubbles_y)
            draw_text("Catch it!", 10, 10)
        end

    -- STATE 3: RESULT
    elseif state == "win" or state == "lose" then
        draw_sprite("rom:/sprites/bubbles.sprite", bubbles_x, bubbles_y)

        if state == "win" then
            draw_text("YUM!", 140, 110)
        else
            draw_sprite("rom:/sprites/sushi.sprite", sushi_x, sushi_y)
            draw_text("MISSED!", 130, 110)
        end

        if current_time - state_timer > 1500 then
            end_frame()
            return score
        end
    end

    end_frame()
end