#include "kos.h";
#include <stdlib.h>;
#include <algorithm>;
#include <stdarg.h>;
#include "bytechars.h";

#define PACK_PIXEL(r, g, b) ( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3) )

struct Vector2
{
	int16 horizontal;
	int16 vertical;

	Vector2 operator +=(const Vector2 table)
	{
		horizontal += table.horizontal;
		vertical += table.vertical;
		return *this;
	}

	Vector2 operator -=(const Vector2 table)
	{
		horizontal -= table.horizontal;
		vertical -= table.vertical;
		return *this;
	}
};

struct Drawable
{
	Vector2 position;
	Vector2 size;
};

struct Ball
{
	Vector2 mom;
	Drawable props;
};

int8 get_paddle_new_mom(const cont_state_t* state, const Drawable* paddle);

bool collision_check();

void draw_obj(Vector2 pos, Vector2 size, uint8 R, uint8 G, uint8 B);

void draw_obj(const Drawable drawable, uint8 R = 255, uint8 G = 255, uint8 B = 255) { draw_obj(drawable.position, drawable.size, R, G, B); };

void draw_string(Vector2 pos, int size_multiplier, int spacing, uint8* data, ...);

void draw_data(Vector2 pos, int size_multiplier, uint8* data);

void draw_score();

uint8* get_score_chars(uint8 number);

void reset();

int16 get_new_vertical_momentum(const Ball* ball, const Drawable* paddle);

const uint8 PADDLE_OFFSET_HORIZONTAL = 20;
const uint16 PADDLE_START_HEIGHT = 200;

const Vector2 PADDLE_SIZE = { 20, 100 };
const Vector2 BALL_SIZE = { 16, 16 };

const uint8 PPM = 10; //PIXELS PER MOVEMENT

const uint16 WIDTH = 640;
const uint16 HEIGHT = 480;

uint8 INITIAL_BALL_SPEED = 7;
const uint8 INCREMENT_BALL_SPEED = 3;

Drawable paddle1;
Drawable paddle2;
Ball ball;

bool started = false;

uint8 score1 = 0;
uint8 score2 = 0;

uint16 hits = 0;

int main()
{
	//init kos
	pvr_init_defaults();
	maple_init();

	//set our video mode
	vid_set_mode(DM_640x480_VGA, PM_RGB565);

	maple_device* controller1;
	maple_device* controller2;

	while (controller1 == NULL)
	{
		controller1 = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	}

	while (controller2 == NULL)
	{
		controller2 = maple_enum_type(1, MAPLE_FUNC_CONTROLLER);
	}

	reset();

	while (1) {
		cont_state_t* state1 = (cont_state_t*)maple_dev_status(controller1);
		cont_state_t* state2 = (cont_state_t*)maple_dev_status(controller2);
		
		while (!started)
		{
			started = state1->buttons & CONT_START;
			thd_sleep(5);
		}

		if (state1->buttons & CONT_B)
		{
			reset();
			score1 = 0;
			score2 = 0;

			continue;
		}

		int8 paddle1Mom = get_paddle_new_mom(state1, &paddle1);
		int8 paddle2Mom = get_paddle_new_mom(state2, &paddle2);

		paddle1.position.vertical += paddle1Mom;
		paddle2.position.vertical += paddle2Mom;

		if (!collision_check())
		{
			continue;
		}

		vid_clear(0, 0, 0);

		draw_obj(paddle1);
		draw_obj(paddle2);

		draw_obj(ball.props);

		draw_score();

		thd_sleep(20);
	}
	return 0;
}

int8 get_paddle_new_mom(const cont_state_t* state, const Drawable* paddle)
{
	if (state->buttons & CONT_DPAD_DOWN && (paddle->position.vertical + PPM + paddle->size.vertical) <= HEIGHT)
		return PPM;
	else if (state->buttons & CONT_DPAD_UP && (paddle->position.vertical - PPM) >= 0)
		return -PPM;
	return 0;
}

bool collision_check()
{
	uint16 newBallWidth = ball.props.position.horizontal + ball.mom.horizontal;
	uint16 newBallHeight = ball.props.position.vertical + ball.mom.vertical;
	if (newBallWidth < PADDLE_OFFSET_HORIZONTAL)
	{
		score2++;
		ball.props.position = { 0, newBallHeight };

		vid_clear(0, 0, 0);
		draw_obj(paddle1);
		draw_obj(paddle2);

		draw_obj(ball.props, 255, 0, 0);

		draw_score();

		reset();
		return false;
	}
	else if (newBallWidth + BALL_SIZE.horizontal > WIDTH - PADDLE_OFFSET_HORIZONTAL)
	{
		score1++;
		ball.props.position = { WIDTH - BALL_SIZE.horizontal, newBallHeight };

		vid_clear(0, 0, 0);
		draw_obj(paddle1);
		draw_obj(paddle2);

		draw_obj(ball.props, 255, 0, 0);
		
		draw_score();

		reset();
		return false;
	}
	else
	{
		ball.props.position.horizontal += ball.mom.horizontal;
	}

	if (newBallHeight < 0 || newBallHeight + BALL_SIZE.vertical > HEIGHT)
	{
		ball.mom.vertical = -ball.mom.vertical;
		ball.props.position.vertical + ball.mom.vertical;
	}
	else
	{
		ball.props.position.vertical = newBallHeight;
	}

	uint16 paddle2_real_horizontal_pos = paddle2.position.horizontal - paddle2.size.horizontal;

	if (ball.mom.horizontal > 0 && (ball.props.position.horizontal > paddle2_real_horizontal_pos && ball.props.position.horizontal < paddle2_real_horizontal_pos + paddle2.size.horizontal) && (ball.props.position.vertical > paddle2.position.vertical && ball.props.position.vertical < paddle2.position.vertical + paddle2.size.vertical))
	{
		ball.mom.horizontal = -ball.mom.horizontal;
		hits++;
		if (hits >= 10)
		{
			ball.mom.horizontal += INCREMENT_BALL_SPEED;
			hits = 0;
		}
		ball.mom.vertical = get_new_vertical_momentum(&ball, &paddle2);
	}
	if (ball.mom.horizontal < 0 && (ball.props.position.horizontal > paddle1.position.horizontal && ball.props.position.horizontal < paddle1.position.horizontal + paddle1.size.horizontal) && (ball.props.position.vertical > paddle1.position.vertical && ball.props.position.vertical < paddle1.position.vertical + paddle1.size.vertical))
	{
		ball.mom.horizontal = -ball.mom.horizontal;
		hits++;
		if (hits >= 10)
		{
			ball.mom.horizontal += INCREMENT_BALL_SPEED;
			hits = 0;
		}
		ball.mom.vertical = get_new_vertical_momentum(&ball, &paddle1);
	}

	return true;
}

void draw_obj(Vector2 pos, Vector2 size, uint8 R, uint8 G, uint8 B)
{
	for (uint16 j = 0; j < size.vertical; j++)
	{
		for (uint16 i = 0; i < size.horizontal; i++)
		{
			vram_s[(pos.horizontal + i) + ((pos.vertical + j) * WIDTH)] = PACK_PIXEL(R, G, B);
		}
	}
}

void draw_string(Vector2 pos, int size_multiplier, int spacing, uint8* data, ...)
{
	va_list arguments;
	for (va_start(arguments, data); data != NULL; data = va_arg(arguments, uint8*))
	{
		draw_data(pos, size_multiplier, data);
		pos += { (8 * (size_multiplier)) + spacing, 0 };
	}

	va_end(arguments);
}

void draw_data(Vector2 pos, int size_multiplier, uint8* data)
{
	for (uint16 j = 0; j < 8; j++)
	{
		for (uint16 i = 0; i < 8; i++)
		{
			int color = data[(j * 8) + i] * 255;

			for (uint8 mx = 0; mx < size_multiplier; mx++)
			{
				for (uint my = 0; my < size_multiplier; my++)
				{
					vram_s[(mx + pos.horizontal + (i * size_multiplier)) + ((my + pos.vertical + (j * size_multiplier)) * WIDTH)] = PACK_PIXEL(color, color, color);
				}
			}
		}
	}
}

uint8* get_score_chars(uint8 number)
{
	switch (number)
	{
	default:
	case 0:
		return D0;
	case 1:
		return D1;
	case 2:
		return D2;
	case 3:
		return D3;
	case 4:
		return D4;
	case 5:
		return D5;
	case 6:
		return D6;
	case 7:
		return D7;
	case 8:
		return D8;
	case 9:
		return D9;
	}
}

void draw_score()
{
	draw_string({ WIDTH / 3, 4 }, 8, 10, get_score_chars(score1), separator, get_score_chars(score2), NULL);
}

void reset()
{
	paddle1.position = { PADDLE_OFFSET_HORIZONTAL, PADDLE_START_HEIGHT };
	paddle1.size = PADDLE_SIZE;

	paddle2.position = { WIDTH - PADDLE_OFFSET_HORIZONTAL - PADDLE_SIZE.horizontal, PADDLE_START_HEIGHT };
	paddle2.size = PADDLE_SIZE;

	ball.mom = { INITIAL_BALL_SPEED, (std::rand() % 28) - 14 };
	ball.props.position = { WIDTH / 2, std::rand() % HEIGHT - BALL_SIZE.vertical };
	ball.props.size = BALL_SIZE;

	hits = 0;

	draw_string({ 20, 20 }, 4, 6, P, R, E, S, S, NULL);
	draw_string({ 20, 62 }, 4, 6, S, T, A, R, T, NULL);

	started = false;
}

int16 get_new_vertical_momentum(const Ball* ball, const Drawable* paddle) //WARNING: i know that this is terrible but im not good at maths :p
{
	int16 ballMiddle = ball->props.position.vertical + ball->props.size.vertical / 2;
	uint16 paddleVertPos = paddle->position.vertical;

	int16 ballHoriMom = std::abs(ball->mom.horizontal);

	if (ballMiddle >= paddleVertPos && ballMiddle <= paddleVertPos + 10)
		return -(ballHoriMom * 2);
	else if (ballMiddle >= paddleVertPos + 10 && ballMiddle <= paddleVertPos + 20)
		return -((ballHoriMom * 2) - (ballHoriMom / 2));
	else if (ballMiddle >= paddleVertPos + 20 && ballMiddle <= paddleVertPos + 30)
		return -((ballHoriMom * 2) - (ballHoriMom / 2) * 2);
	else if (ballMiddle >= paddleVertPos + 30 && ballMiddle <= paddleVertPos + 40)
		return -((ballHoriMom * 2) - (ballHoriMom / 2) * 3);
	else if (ballMiddle >= paddleVertPos + 40 && ballMiddle <= paddleVertPos + 50)
		return -((ballHoriMom * 2) - (ballHoriMom / 2) * 6);
	else if (ballMiddle >= paddleVertPos + 50 && ballMiddle <= paddleVertPos + 60)
		return (ballHoriMom * 2) - (ballHoriMom / 2) * 6;
	else if (ballMiddle >= paddleVertPos + 60 && ballMiddle <= paddleVertPos + 70)
		return (ballHoriMom * 2) - (ballHoriMom / 2) * 3;
	else if (ballMiddle >= paddleVertPos + 70 && ballMiddle <= paddleVertPos + 80)
		return (ballHoriMom * 2) - (ballHoriMom / 2) * 2;
	else if (ballMiddle >= paddleVertPos + 80 && ballMiddle <= paddleVertPos + 90)
		return (ballHoriMom * 2) - (ballHoriMom / 2);
	else if (ballMiddle >= paddleVertPos + 90 && ballMiddle <= paddleVertPos + 100)
		return ballHoriMom * 2;
}