/* ************************************************************
 * SOR 22-23: simple example of image processing
 *            Very crude and basic example, only for illustartion
 * 			 Actual implementations should be far better than this
 *
 * Paulo Pedreiras, Nov 2022
 *
 ************************************************************** */

/* The usual includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

/* Some defines that are usefull to make the SW more readable */
/* and adaptable */
#define IMGWIDTH 16			  /* Square image. Side size, in pixels*/
#define BACKGROUND_COLOR 0x00 /* Color of the background */
#define GUIDELINE_COLOR 0xFF  /* Guideline color */
#define OBSTACLE_COLOR 0x80	  /* Obstacle color */
#define GN_ROW 0			  /* Row to look for the guiode line - close */
#define GF_ROW (IMGWIDTH - 1) /* Row to look for the guiode line - far */
#define NOB_ROW 3			  /* Row to look for near obstacles */
#define NOB_COL 5			  /* Col to look for near obstacles */
#define NOB_WIDTH 5			  /* WIDTH of the sensor area */

/* One example image. In raw/gray format an image is an array of
 * bytes, one per pixel, with values that represent intensity and range
 * from black (0x00) to bright white (0xFF).
 * In the example image below you can see the image
 * pixels/bytes organized in rows-columns (matrix format). It is not
 * necessary for the SW. I put it this way just to facilitate
 * this explanation.
 * You can see on the 8th column a white stripe (column of pixels with
 * 0xFF). On the bottom-righ, two last rows, you can see two pixels
 * of gray color, that represent two obstacles.
 */
uint8_t vertical_guide_image_data[IMGWIDTH][IMGWIDTH] =
	{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80}};

uint8_t plus45_guide_image_data[IMGWIDTH][IMGWIDTH] =
	{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00},
	 {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00}};

uint8_t minus45_guide_image_data[IMGWIDTH][IMGWIDTH] =
	{{0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xFF, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF}};
/* Function that detects he position and agle of the guideline */
/* Very crude implemenation. Just for illustration purposes */
int guideLineSearch(uint8_t imageBuf[IMGWIDTH][IMGWIDTH], int16_t *pos, float *angle)
{
	int i, gf_pos;

	/* Inits */
	*pos = -1;
	gf_pos = -1;

	/* Search for guideline pos - Near*/
	for (i = 0; i < IMGWIDTH; i++)
	{
		if (imageBuf[GN_ROW][i] == GUIDELINE_COLOR)
		{
			*pos = i;
			break;
		}
	}

	/* Search for guideline pos - Far*/
	for (i = 0; i < IMGWIDTH; i++)
	{
		if (imageBuf[GF_ROW][i] == GUIDELINE_COLOR)
		{
			gf_pos = i;
			break;
		}
	}

	if (*pos == -1 || gf_pos == -1)
	{
		printf("Failed to find guideline pos=%d, gf_pos=%d", *pos, gf_pos);
		return -1;
	}

	/* Approach very grossly the angle (NOT a valid solution - just for testing ) */
	if (*pos == gf_pos)
	{
		*angle = 0;
	}
	else
	{
		int pos_delta = *pos - gf_pos;
		if (pos_delta > 0)
			pos_delta++;
		else
			pos_delta--;
		*angle = acos(16 / sqrt(pow(16, 2) + pow(pos_delta, 2)));
		if (pos_delta < 0)
			*angle = -*angle;
	}

	return 0;
}

/* Function to look for closeby obstacles */
/* Not implemented */
int nearObstSearch(uint8_t imageBuf[IMGWIDTH][IMGWIDTH])
{

	return 0;
}

/* Function that counts obstacles.
/* Crude version. Only works if one obstacle per row at max. */
int obstCount(uint8_t imageBuf[IMGWIDTH][IMGWIDTH])
{
	int i, j, nobs;

	/* Inits */
	nobs = 0;

	/* Search for obstacles. Crude version. Only works if one obstacle per row at max*/
	for (j = 0; j < IMGWIDTH; j++)
	{
		int inObs = 0;
		for (i = 0; i < IMGWIDTH; i++)
		{
			if (imageBuf[j][i] == OBSTACLE_COLOR)
			{
				inObs++;
			}
			else if (inObs > 1)
			{
				nobs++;
				inObs = 0;
			}
		}
		if (inObs > 1)
			nobs++;
	}

	return nobs;
}

/* Main function */
int main()
{
	int res;

	int16_t pos;
	float angle;

	printf("Test for image processing algorithms \n\r");

	printf("Detecting position and guideline angle ...");
	res = guideLineSearch(plus45_guide_image_data, &pos, &angle);
	printf("Robot position=%d, guideline angle = %f\n\r", pos, angle);

	printf("Detecting number of obstacles ...");
	res = obstCount(vertical_guide_image_data);
	printf("%d obstacles detected\n\r", res);
}
