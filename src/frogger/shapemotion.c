/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#define GREEN_LED BIT6

typedef struct MovLayer_s {
    Layer *layer;
    Vec2 velocity;
    struct MovLayer_s *next;
} MovLayer;

/*********************************************************************************/
// My code for frogger

/* Grass Segment Rectangle Shapes */
AbRect grassShape1 = {abRectGetBounds, abRectCheck, {screenWidth/2, screenHeight/14}};
AbRect grassShape2 = {abRectGetBounds, abRectCheck, {screenWidth/2, screenHeight/14}};
AbRect grassShape3 = {abRectGetBounds, abRectCheck, {screenWidth/2, screenHeight/14}};
AbRect grassShape4 = {abRectGetBounds, abRectCheck, {screenWidth/2, screenHeight/14}};

/* Road Segment Rectangle Shapes */
AbRect roadShape1 = {abRectGetBounds, abRectCheck, {screenWidth/2, screenHeight/14}};
AbRect roadShape2 = {abRectGetBounds, abRectCheck, {screenWidth/2, screenHeight/14}};
AbRect roadShape3 = {abRectGetBounds, abRectCheck, {screenWidth/2, screenHeight/14}};

/* Car Arrow Shapes */
AbRArrow carShape1 = {abRArrowGetBounds, abRArrowCheck, screenHeight/7};
AbRArrow carShape2 = {abRArrowGetBounds, abRArrowCheck, screenHeight/7};
AbRArrow carShape3 = {abRArrowGetBounds, abRArrowCheck, screenHeight/7};
//AbRect carShape1 = {abRectGetBounds, abRectCheck, {10, screenHeight/14-4}};
//AbRect carShape2 = {abRectGetBounds, abRectCheck, {40, screenHeight/14-4}};
//AbRect carShape3 = {abRectGetBounds, abRectCheck, {20, screenHeight/14-4}};

/*
 * Lane x pixel positions = [21, 41, 64, 87, 107]
 * Lane y pixel positions = [17, 39, 61, 83, 105, 127, 149]
 * Grass lane y positions = [17,61,105,149]
 * Road lane y positions = [39,83,127]
 * Formula for calculating x pixel position:
 * screenHeight/6*i // i => 1...5
 * Formula for calculating y pixel position:
 * (h*i)+(h/2)+(screenHeight%7) // h => height of lane, i => 0..<7
 */

#define START_X 2 // Starting x lane pos index for player
#define START_Y 0 // Starting y lane pos index for player

u_char lanePosX[5] = {21,41,64,87,107};
u_char lanePosY[7] = {17,39,61,83,105,127,149};

u_char frogPosInd_x = START_X; // Player x position index (lookup screen coordinate in lanePosX)
u_char frogPosInd_y = START_Y; // Player y position index (lookup screen coordinate in lanePosY)

/* Grass Shape Layers */
Layer grassLayer1 = {(AbShape*)&grassShape1, {64,  17}, {0, 0}, {0, 0}, COLOR_PURPLE, 0};
Layer grassLayer2 = {(AbShape*)&grassShape2, {64,  61}, {0, 0}, {0, 0}, COLOR_PURPLE, &grassLayer1};
Layer grassLayer3 = {(AbShape*)&grassShape3, {64, 105}, {0, 0}, {0, 0}, COLOR_PURPLE, &grassLayer2};
Layer grassLayer4 = {(AbShape*)&grassShape4, {64, 149}, {0, 0}, {0, 0}, COLOR_PURPLE, &grassLayer3}; // Highest precedence grass layer

/* Road Shape Layers */
Layer roadLayer1 = {(AbShape*)&roadShape1, {64,  39}, {0, 0}, {0, 0}, COLOR_BLACK, &grassLayer4};
Layer roadLayer2 = {(AbShape*)&roadShape2, {64,  83}, {0, 0}, {0, 0}, COLOR_BLACK, &roadLayer1};
Layer roadLayer3 = {(AbShape*)&roadShape3, {64, 127}, {0, 0}, {0, 0}, COLOR_BLACK, &roadLayer2}; // Highest precedence road layer

/* Car Layers */
Layer carLayer1 = {(AbShape*)&carShape1, {0,  39}, {0, 0}, {0, 0}, COLOR_BLUE, &roadLayer3};
Layer carLayer2 = {(AbShape*)&carShape2, {0,  83}, {0, 0}, {0, 0}, COLOR_ORANGE, &carLayer1};
Layer carLayer3 = {(AbShape*)&carShape3, {0, 127}, {0, 0}, {0, 0}, COLOR_RED, &carLayer2}; // Highest precedence car layer

Layer *carLayers[3] = {&carLayer1, &carLayer2, &carLayer3};

/* Frog Shape and Layer */
Layer frogLayer = {(AbShape*)&circle6, {64, 17}, {0, 0}, {0, 0}, COLOR_GREEN, &carLayer3}; // Will have the highest precedence of all layers
MovLayer frog = {&frogLayer, {0,0}, 0};

/* Moving Car Layers */
MovLayer car1 = {&carLayer1, {3, 0}, &frog};
MovLayer car2 = {&carLayer2, {2, 0}, &car1};
MovLayer car3 = {&carLayer3, {4, 0}, &car2}; // Highest precedence car moving layer

Region gameViewBoundary = {
	{0,0}, // Top Left Corner
	{screenWidth,screenHeight} // Bottom Right Corner
};

/*********************************************************************************/

// Setup Board and CPU Settings
void configure() {
    P1DIR |= GREEN_LED; // Green led on when CPU on		
	P1OUT |= GREEN_LED;

	configureClocks();
	lcd_init();
	p2sw_init(15); // Initialize 4 available board buttons using bit mask

	layerInit(&frogLayer); // This statement is required to initialize the drawing of all shapes and layers
	layerDraw(&frogLayer);
	
	enableWDTInterrupts(); // enable periodic interrupt
	or_sr(0x8); // GIE (enable interrupts)
}

// Draws moving layers
void movLayerDraw(MovLayer *movLayers, Layer *layers) {
	int row, col;
	MovLayer *movLayer;

	and_sr(~8);	// disable interrupts (GIE off)
	for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { // Increment moving layer positions
		Layer *l = movLayer->layer;
		l->posLast = l->pos;
		l->pos = l->posNext;
	}
	or_sr(8); // enable interrupts (GIE on)

	for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { // Redraw standard layers
		Region bounds;
		layerGetBounds(movLayer->layer, &bounds);
		lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
		for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
			for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
				Vec2 pixelPos = {col, row};
				u_int color = bgColor;
				Layer *probeLayer;
				for (probeLayer = layers; probeLayer; probeLayer = probeLayer->next) { // probe all layers, in order
					if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
						color = probeLayer->color;
						break; 
					}
				}
				lcd_writeColor(color); 
			}
		}
	}
}	  

// Advance car positions on x axis
void carAdvance(MovLayer *carLayer, Region *fence) {
	Vec2 newPos; Region shapeBoundary;

	while (carLayer) {
		vec2Add(&newPos, &carLayer->layer->posNext, &carLayer->velocity); // Add velocity increment to current position
		abShapeGetBounds(carLayer->layer->abShape, &newPos, &shapeBoundary);
		char carSize = ((AbRArrow*)carLayer->layer->abShape)->size;
		if (&carLayer->velocity > 0 && shapeBoundary.botRight.axes[0] >= fence->botRight.axes[0]) // Car is moving to the right
		  newPos.axes[0] = fence->topLeft.axes[0]-carSize; // Set car to leftmost position on screen
		else if (&carLayer->velocity < 0 && shapeBoundary.topLeft.axes[0] <= fence->topLeft.axes[0]) // Car is moving to the left
			newPos.axes[0] = fence->botRight.axes[0]-3; // Set car to rightmost position on screen
		carLayer->layer->posNext = newPos; // Change layer position
		carLayer = carLayer->next; // Move to next layer
	}
}

// Determines if region2 exists anywhere in region1
char containsRegion(Region *region1, Region *region2) {
	// Region1 corner bound coordinates
	u_char reg1_tlx = region1->topLeft.axes[0]; // Top Left X
	u_char reg1_tly = region1->topLeft.axes[1]; // Top Left Y
	u_char reg1_blx = region1->botRight.axes[0]; // Bottom Left X
	u_char reg1_bly = region1->botRight.axes[1]; // Bottom Left Y
	// Region2 corner bound coordinates
	u_char reg2_tlx = region2->topLeft.axes[0]; // Top Left X
	u_char reg2_tly = region2->topLeft.axes[1]; // Top Left Y
	u_char reg2_blx = region2->botRight.axes[0]; // Bottom Left X
	u_char reg2_bly = region2->botRight.axes[1]; // Bottom Left Y

	if ((reg2_tlx >= reg1_tlx && reg2_tly >= reg2_tly) && (reg2_tlx <= reg1_blx && reg2_tly <= reg1_bly))
		return 1;
	else if ((reg2_blx >= reg1_tlx && reg2_bly >= reg2_tly) && (reg2_blx <= reg1_blx && reg2_bly <= reg1_bly))
		return 1;
	else
		return 0;
}

/* Determines if the any of the frog region's x coordinates resides within the car region. */
char wasHit(Region *carReg, Region *frogReg) {
	// Car x-coordinates
	u_char car_tlx = carReg->topLeft.axes[0];
	u_char car_brx = carReg->botRight.axes[0];
	// Frog x-coordinates
	u_char frog_tlx = frogReg->topLeft.axes[0];
	u_char frog_brx = frogReg->botRight.axes[0];

	if (frog_tlx < car_brx && frog_tlx > car_tlx) return 1; // Player hit from left
	else if (frog_brx < car_brx && frog_brx > car_tlx) return 1; // Player hit from right
	else return 0;
}

/* Moves frog position and redraws layer once its layer is changed. */
void moveFrog(u_char direction) {
	switch (direction) {
		case 1: // Move Frog Left
			if(frogPosInd_x <= 0) return; // Cannot move further than this point
			frogLayer.posNext = (Vec2){lanePosX[--frogPosInd_x], lanePosY[frogPosInd_y]};
			break;
		case 2: // Move Frog Right
			if (frogPosInd_x >= 4) return; // Cannot move further than this point
			frogLayer.posNext = (Vec2){lanePosX[++frogPosInd_x], lanePosY[frogPosInd_y]};
			break;
		case 3: // Move Frog Up
			if (frogPosInd_y <= 0) return; // Cannot move further than this point
			frogLayer.posNext = (Vec2){lanePosX[frogPosInd_x], lanePosY[--frogPosInd_y]};
			break;
		case 4: // Move Frog Down
			if (frogPosInd_y >= 6) return; // Cannot move further than this point
			frogLayer.posNext = (Vec2){lanePosX[frogPosInd_x], lanePosY[++frogPosInd_y]};
			break;
		default:
			return;
	}
}

/* Determines if frog is run over by car (frog bounds exist within bounds of a car) */
char didLose() {
	for (u_char i = 0; i < 3; i++) {
		Layer *carLayer = carLayers[i]; // Check frog position against this car layer
		if (frogLayer.pos.axes[1] != carLayer->pos.axes[1]) continue; // Layers are not in the same lane y position
		Region carBounds, frogBounds;
		abShapeGetBounds(carLayer->abShape, &carLayer->pos, &carBounds);
		abShapeGetBounds(frogLayer.abShape, &frogLayer.pos, &frogBounds);
		if (wasHit(&carBounds, &frogBounds)) return 1; // Player hit by car, player lost
	}
	return 0;
}

/* Player is in the final lane */
char didWin() { return frogPosInd_y >= 6; }

/*********************************************************************************/

u_int bgColor = COLOR_BLACK; // Game background color
int redrawScreen = 1; // Boolean for whether screen needs to be redrawn
u_int prevPress; // Switch mask for determining which buttons were previously pressed

/** 
 * Initializes everything, enables interrupts and green LED, 
 * and handles the rendering for the screen
 */
void main() {
	configure();

	while (1) { 
		while (!redrawScreen) { // Pause CPU if screen doesn't need updating
			P1OUT &= ~GREEN_LED; // Turn Green led off while CPU is off
			or_sr(0x10); // Turn CPU off
		}
		P1OUT |= GREEN_LED; // Turn Green led on while CPU is on
		redrawScreen = 0;
		movLayerDraw(&car3, &frogLayer); // Begin drawing layers
	}
}

// Watchdog timer interrupt handler. 15 interrupts/sec
void wdt_c_handler() {
	static short count = 0;
	u_int pressed = p2sw_read(); // Read switch input from board
	P1OUT |= GREEN_LED; // Green LED on when cpu on
	if (++count == 15) {
		if (didWin()) {  // Check if player's frog is in the last lane
			Vec2 stop = {0,0}; // Stop cars from moving
			car1.velocity = stop;
			car2.velocity = stop;
			car3.velocity = stop;
			//p2sw_init(0); // Turn off switches
		}
		if (didLose()) { // Check if player's frog was hit by a car
			Vec2 start = (Vec2){lanePosX[frogPosInd_x = START_X], lanePosY[frogPosInd_y = START_Y]};
			frogLayer.posNext = start; // Reset player position to starting point
		}
		carAdvance(&car3, &gameViewBoundary); // Advance cars to their next position

		u_int switches = ~pressed; // Actual pressed swtich values
		u_int changed = prevPress ^ switches; // Which switches were changed from the previous state
		if (switches & 4 && changed & 4) moveFrog(4); // Move frog down
		if (switches & 2 && changed & 2) moveFrog(3); // Move frog up
		if (switches & 1 && changed & 1) moveFrog(1); // Move frog left
		if (switches & 8 && changed & 8) moveFrog(2); // Move frog right
		prevPress = switches;
		
		if (pressed) redrawScreen = 1;
		count = 0;
	} 
	P1OUT &= ~GREEN_LED; // Green LED off when cpu off
}
