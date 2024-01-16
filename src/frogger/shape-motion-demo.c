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
#define RED_LED BIT0

typedef struct MovLayer_s {
    Layer *layer;
    Vec2 velocity;
    struct MovLayer_s *next;
} MovLayer;

/*********************************************************************************/
// My code for frogger

u_char midpointHeight = screenHeight / 2
u_char midpointWidth = screenWidth / 2;
u_char laneHeight = screenHeight / 7;

/* Grass Segment Rectangle Shapes */
AbRect grassShape1 = {abRectGetBounds, abRectCheck, {screenWidth, laneHeight}};
AbRect grassShape2 = {abRectGetBounds, abRectCheck, {screenWidth, laneHeight}};
AbRect grassShape3 = {abRectGetBounds, abRectCheck, {screenWidth, laneHeight}};
AbRect grassShape4 = {abRectGetBounds, abRectCheck, {screenWidth, laneHeight}};

/* Road Segment Rectangle Shapes */
AbRect roadShape1 = {abRectGetBounds, abRectCheck, {screenWidth, laneHeight}};
AbRect roadShape2 = {abRectGetBounds, abRectCheck, {screenWidth, laneHeight}};
AbRect roadShape3 = {abRectGetBounds, abRectCheck, {screenWidth, laneHeight}};

/* Car Arrow Shapes */
AbArrow carShape1 = {abRArrowGetBounds, abRArrowCheck, laneHeight};
AbArrow carShape2 = {abRArrowGetBounds, abRArrowCheck, laneHeight};
AbArrow carShape3 = {abRArrowGetBounds, abRArrowCheck, laneHeight};

/*
 * Horizontal lane y positions = [17, 39, 61, 83, 105, 127, 149]
 * Grass lane y positions = [17,61,105,149]
 * Road lane y positions = [39,83,127]
 * Formula for calculating position:
 * (h*i)+(h/2)+(screenHeight%7) // h => height of lane
 */

/* Grass Shape Layers */
Layer grassLayer1 = {&grassShape1, {64,  17}, {0, 0}, {0, 0}, COLOR_PURPLE, 0};
Layer grassLayer2 = {&grassShape2, {64,  61}, {0, 0}, {0, 0}, COLOR_PURPLE, &grassLayer1};
Layer grassLayer3 = {&grassShape3, {64, 105}, {0, 0}, {0, 0}, COLOR_PURPLE, &grassLayer2};
Layer grassLayer4 = {&grassShape4, {64, 149}, {0, 0}, {0, 0}, COLOR_PURPLE, &grassLayer3}; // Highest precedence grass layer

/* Road Shape Layers */
Layer roadLayer1 = {&roadShape1, {64,  39}, {0, 0}, {0, 0}, COLOR_BLACK, &grassLayer4};
Layer roadLayer2 = {&roadShape2, {64,  83}, {0, 0}, {0, 0}, COLOR_BLACK, &roadLayer1};
Layer roadLayer3 = {&roadShape3, {64, 127}, {0, 0}, {0, 0}, COLOR_BLACK, &roadLayer2}; // Highest precedence road layer

/* Car Layers */
Layer carLayer1 = {&carShape1, {0,  39}, {0, 0}, {0, 0}, COLOR_BLUE, &roadLayer1};
Layer carLayer2 = {&carShape2, {0,  83}, {0, 0}, {0, 0}, COLOR_ORANGE, &carLayer1};
Layer carLayer3 = {&carShape3, {0, 127}, {0, 0}, {0, 0}, COLOR_RED, &carLayer2}; // Highest precedence car layer

/* Frog Shape and Layer */
AbCircle frogShape = &circle10; // Frogger character
Layer frogLayer = {&frogShape, {64, 17}, {0, 0}, {0, 0}, COLOR_GREEN, &carLayer3}; // Will have the highest precedence of all layers

/* Moving Car Layers */
MovLayer car1 = {&carLayer1, {3, 0}, 0};
MovLayer car2 = {&carLayer2, {1, 0}, &car1};
MovLayer car3 = {&carLayer3, {4, 0}, &car2}; // Highest precedence car moving layer

Region gameViewBoundary = {
	{0,0}, // Top Left Corner
	{screenWidth,screenHeight} // Bottom Right Corner
};

/*********************************************************************************/

AbRectOutline fieldOutline = {	/* playing field */
	abRectOutlineGetBounds, abRectOutlineCheck,   
	{screenWidth/2 - 10, screenHeight/2 - 10}
};

Layer fieldLayer = {		/* playing field as a layer */
	(AbShape *) &fieldOutline,
	{screenWidth/2, screenHeight/2},/**< center */
	{0,0}, {0,0},				    /* last & next pos */
	COLOR_WHITE,
	&layer3
};

/*********************************************************************************/

// Setup Board and CPU Settings
void configure() {
    P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
	P1OUT |= GREEN_LED;

	configureClocks();
	lcd_init();
	p2sw_init(1);

	layerInit(&layer0);
	layerDraw(&layer0);
	layerGetBounds(&fieldLayer, &fieldFence);

	enableWDTInterrupts();      /**< enable periodic interrupt */
	or_sr(0x8);	              /**< GIE (enable interrupts) */
}

// Draws moving layers
void movLayerDraw(MovLayer *movLayers, Layer *layers) {
	int row, col;
	MovLayer *movLayer;

	and_sr(~8);	/**< disable interrupts (GIE off) */
	for (movLayer = movLayers; movLayer; movLayer = movLayer->next) {
		Layer *l = movLayer->layer;
		l->posLast = l->pos;
		l->pos = l->posNext;
	}
	or_sr(8);	/**< enable interrupts (GIE on) */


	for (movLayer = movLayers; movLayer; movLayer = movLayer->next) {
		Region bounds;
		layerGetBounds(movLayer->layer, &bounds);
		lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
		for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
			for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
				Vec2 pixelPos = {col, row};
				u_int color = bgColor;
				Layer *probeLayer;
				for (probeLayer = layers; probeLayer; probeLayer = probeLayer->next) { /* probe all layers, in order */
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


/** Advances a moving shape layer within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence) {
	Vec2 newPos;
	Region shapeBoundary;
	for (; ml; ml = ml->next) {
		vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
		abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
		for (u_char axis = 0; axis < 2; axis ++) { // For each directional axis (X and Y directions)
			if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
				(shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis])) { // Shape layer is outside of fence
				int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
				newPos.axes[axis] += (2*velocity);
			}
		}
		ml->layer->posNext = newPos;
	}
}

// Advance car positions on x axis
void carAdvance(MovLayer *carLayer, Region *fence) {
	Vec2 newPos;
	Region shapeBoundary;

	while (carLayer) {
		vec2Add(&newPos, &carLayer->layer->posNext, &carLayer->velocity); // Add velocity increment to current position
		abShapeGetBounds(carLayer->layer->abShape, &newPos, &shapeBoundary);
		if (carLayer->velocity > 0 && shapeBoundary.botRight.axes[0] >= fence->botRight.axes[0]) { // Car is moving to the right
			newPos[0] = fence->topLeft.axes[0]; // Set car to leftmost position on screen
		}
		else if (carLayer->velocity < 0 && shapeBoundary.topLeft.axes[0] <= fence->topLeft.axes[0]) { // Car is moving to the left
			newPos[0] = fence->botRight.axes[0]; // Set car to rightmost position on screen
		}

		carLayer->layer->posNext = newPos; // Change layer position
		carLayer = carLayer->next // Move to next layer
	}
}

// Determines if region2 exists anywhere in region1
char containsRegion(Region *region1, Region *region2) {
	// Region1 corner bound coordinates
	u_char reg1_tlx = region1->topLeft.axes[0]; // Top Left X
	u_char reg1_tly = region1->topLeft.axes[1]; // Top Left Y
	u_char reg1_blx = region1->botLeft.axes[0]; // Bottom Left X
	u_char reg1_bly = region1->botLeft.axes[1]; // Bottom Left Y
	// Region2 corner bound coordinates
	u_char reg2_tlx = region2->topLeft.axes[0]; // Top Left X
	u_char reg2_tly = region2->topLeft.axes[1]; // Top Left Y
	u_char reg2_blx = region2->botLeft.axes[0]; // Bottom Left X
	u_char reg2_bly = region2->botLeft.axes[1]; // Bottom Left Y

	if ((reg2_tlx >= reg1_tlx && reg2_tly >= reg2_tly) && (reg2_tlx <= reg1_blx && reg2_tly <= reg1_bly))
		return 1;
	else if ((reg2_blx >= reg1_tlx && reg2_bly >= reg2_tly) && (reg2_blx <= reg1_blx && reg2_bly <= reg1_bly))
		return 1;
	else
		return 0;
}

// Determines if frog is run over by car (frog bounds exist within bounds of a car)
char didLose(Layer *frogLayer, Layer *carLayer) {
	while (carLayer) {
		Region carBounds;
		Region frogBounds;
		abShapeGetBounds(carLayer->layer->abShape, &carLayer->layer->pos, &carBounds);
		abShapeGetBounds(frogBounds->layer->abShape, &frogLayer->layer->pos, &frogBounds);
		if (containsRegion(&carBounds, &frogBounds)) return 1; // Player hit by car, game over
		carLayer = carLayer->next;
	}
}


u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main() {
	configure();

	for(;;) { 
		while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
			P1OUT &= ~GREEN_LED;    /**< Turn Green led off while CPU is off */
			or_sr(0x10);	      /**< Turn CPU off */
		}
		P1OUT |= GREEN_LED;       /**< Turn Green led on while CPU is on */
		redrawScreen = 0;
		movLayerDraw(&ml0, &layer0);
	}
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler() {
	static short count = 0;
	P1OUT |= GREEN_LED; // Green LED on when cpu on
    count++;
	if (count == 15) {
		mlAdvance(&ml0, &fieldFence);
		// if (didLose()) {}
		// carAdvance(&car3);
		if (p2sw_read())
			redrawScreen = 1;
		count = 0;
	} 
	P1OUT &= ~GREEN_LED; // Green LED off when cpu off
}