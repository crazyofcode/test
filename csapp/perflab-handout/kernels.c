/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following team struct 
 */
team_t team = {
    "novi",              /* Team name */

    "Harry",     /* First member full name */
    "bovik@gmail.com",  /* First member email address */

    "serein",                   /* Second member full name (leave blank if none) */
    "1710338943@qq.com"                    /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/* 
 * naive_rotate - The naive baseline version of rotate 
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i,j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

/* 
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j,k;

    for (i = 0; i < dim; i+=8){
        for (j = 0; j < dim; j+=8)
        {
            for(k=i;k<i+8;k++)
            {
                dst[RIDX(dim-1-j, k, dim)] = src[RIDX(k, j, dim)];
                dst[RIDX(dim-2-j, k, dim)] = src[RIDX(k, j+1, dim)];
                dst[RIDX(dim-3-j, k, dim)] = src[RIDX(k, j+2, dim)];
                dst[RIDX(dim-4-j, k, dim)] = src[RIDX(k, j+3, dim)];
                dst[RIDX(dim-5-j, k, dim)] = src[RIDX(k, j+4, dim)];
                dst[RIDX(dim-6-j, k, dim)] = src[RIDX(k, j+5, dim)];
                dst[RIDX(dim-7-j, k, dim)] = src[RIDX(k, j+6, dim)];
                dst[RIDX(dim-8-j, k, dim)] = src[RIDX(k, j+7, dim)];
            }

        }
    }
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_rotate_functions() 
{
    add_rotate_function(&naive_rotate, naive_rotate_descr);   
    add_rotate_function(&rotate, rotate_descr);   
    /* ... Register additional test functions here */
}


/***************
 * SMOOTH KERNEL
 **************/

/***************************************************************
 * Various typedefs and helper functions for the smooth function
 * You may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct {
    int red;
    int green;
    int blue;
    int num;
} pixel_sum;

/* Compute min and max of two integers, respectively */
static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

/* 
 * initialize_pixel_sum - Initializes all fields of sum to 0 
 */
static void initialize_pixel_sum(pixel_sum *sum) 
{
    sum->red = sum->green = sum->blue = 0;
    sum->num = 0;
    return;
}

/* 
 * accumulate_sum - Accumulates field values of p in corresponding 
 * fields of sum 
 */
static void accumulate_sum(pixel_sum *sum, pixel p) 
{
    sum->red += (int) p.red;
    sum->green += (int) p.green;
    sum->blue += (int) p.blue;
    sum->num++;
    return;
}

/* 
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel 
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
    current_pixel->red = (unsigned short) (sum.red/sum.num);
    current_pixel->green = (unsigned short) (sum.green/sum.num);
    current_pixel->blue = (unsigned short) (sum.blue/sum.num);
    return;
}

/* 
 * avg - Returns averaged pixel value at (i,j) 
 */
static pixel avg(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = max(i-1, 0); ii <= min(i+1, dim-1); ii++) 
	for(jj = max(j-1, 0); jj <= min(j+1, dim-1); jj++) 
	    accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);

    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}

/******************************************************
 * Your different versions of the smooth kernel go here
 ******************************************************/

/*
 * naive_smooth - The naive baseline version of smooth 
 */
char naive_smooth_descr[] = "naive_smooth: Naive baseline implementation";
void naive_smooth(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    
    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
}

/*
 * smooth - Your current working version of smooth. 
 * IMPORTANT: This is the version you will be graded on
 */

/*
static inline void set_corner(int cc, pixel *src, pixel *dst, int a1, int a2, int a3){
    dst[cc].blue = (src[cc].blue+src[a1].blue+src[a2].blue+src[a3].blue) >> 2;
    dst[cc].green = (src[cc].green+src[a1].green+src[a2].green+src[a3].green) >> 2;
    dst[cc].red = (src[cc].red+src[a1].red+src[a2].red+src[a3].red) >> 2;
}
static inline void set_top(int dim, pixel *src, pixel *dst, int j){
    dst[j].blue = (src[j].blue+src[j+dim].blue+src[j-1].blue+src[j+1].blue+src[j+dim-1].blue+src[j+dim+1].blue)/6;
    dst[j].green = (src[j].green+src[j+dim].green+src[j-1].green+src[j+1].green+src[j+dim-1].green+src[j+dim+1].green)/6;
    dst[j].red = (src[j].red+src[j+dim].red+src[j-1].red+src[j+1].red+src[j+dim-1].red+src[j+dim+1].red)/6;
}
static inline void set_bottom(int dim, pixel *src, pixel *dst, int j){
    dst[j].blue = (src[j].blue+src[j-dim].blue+src[j-1].blue+src[j+1].blue+src[j-dim-1].blue+src[j-dim+1].blue)/6;
    dst[j].green = (src[j].green+src[j-dim].green+src[j-1].green+src[j+1].green+src[j-dim-1].green+src[j-dim+1].green)/6;
    dst[j].red = (src[j].red+src[j-dim].red+src[j-1].red+src[j+1].red+src[j-dim-1].red+src[j-dim+1].red)/6;
}
static inline void set_left(int dim, pixel *src, pixel *dst, int i){
    dst[i].blue = (src[i].blue+src[i-dim].blue+src[i-dim+1].blue+src[i+1].blue+src[i+dim].blue+src[i+dim+1].blue)/6;
    dst[i].green = (src[i].green+src[i-dim].green+src[i-dim+1].green+src[i+1].green+src[i+dim].green+src[i+dim+1].green)/6;
    dst[i].red = (src[i].red+src[i-dim].red+src[i-dim+1].red+src[i+1].red+src[i+dim].red+src[i+dim+1].red)/6;
}
static inline void set_right(int dim, pixel *src, pixel *dst, int i){
    dst[i].blue = (src[i].blue+src[i-dim].blue+src[i-dim-1].blue+src[i-1].blue+src[i+dim].blue+src[i+dim-1].blue)/6;
    dst[i].green = (src[i].green+src[i-dim].green+src[i-dim-1].green+src[i-1].green+src[i+dim].green+src[i+dim-1].green)/6;
    dst[i].red = (src[i].red+src[i-dim].red+src[i-dim-1].red+src[i-1].red+src[i+dim].red+src[i+dim-1].red)/6;
}
static inline void set_in(int dim, pixel *src, pixel *dst, int k){
    dst[k].blue = (src[k].blue+src[k-1].blue+src[k+1].blue+src[k+dim-1].blue+src[k+dim].blue+src[k+dim+1].blue+src[k-dim-1].blue+src[k-dim].blue+src[k-dim+1].blue)/9;
    dst[k].green = (src[k].green+src[k-1].green+src[k+1].green+src[k+dim-1].green+src[k+dim].green+src[k+dim+1].green+src[k-dim-1].green+src[k-dim].green+src[k-dim+1].green)/9;
    dst[k].red = (src[k].red+src[k-1].red+src[k+1].red+src[k+dim-1].red+src[k+dim].red+src[k+dim+1].red+src[k-dim-1].red+src[k-dim].red+src[k-dim+1].red)/9;
}
*/
void optimized_smooth(int dim, pixel *src, pixel *dst)
 {

   // +-------------+--------+--------+--------+--------+--------+--------+--------------+
   // |   Left Top  | Edge 0 | Edge 0 | Edge 0 | Edge 0 | Edge 0 | Edge 0 |   Right Top  |
   // +-------------+--------+--------+--------+--------+--------+--------+--------------+
   // |    Edge 1   |        |        |        |        |        |        |    Edge 2    |
   // +-------------+--------+--------+--------+--------+--------+--------+--------------+
   // |    Edge 1   |        |        |        |        |        |        |    Edge 2    |
   // +-------------+--------+--------+--------+--------+--------+--------+--------------+
   // |    Edge 1   |        |        |        |        |        |        |    Edge 2    |
   // +-------------+--------+--------+--------+--------+--------+--------+--------------+
   // |    Edge 1   |        |        |        |        |        |        |    Edge 2    |
   // +-------------+--------+--------+--------+--------+--------+--------+--------------+
   // |    Edge 1   |        |        |        |        |        |        |    Edge 2    |
   // +-------------+--------+--------+--------+--------+--------+--------+--------------+
   // |    Edge 1   |        |        |        |        |        |        |    Edge 2    |
   // +-------------+--------+--------+--------+--------+--------+--------+--------------+
   // | Left Bottom | Edge 3 | Edge 3 | Edge 3 | Edge 3 | Edge 3 | Edge 3 | Right Bottom |
   // +-------------+--------+--------+--------+--------+--------+--------+--------------+

   // Smooth the four corners.
   int curr;

   // Left Top
   dst[0].red   = (src[0].red + src[1].red + src[dim].red + src[dim + 1].red) >> 2;
   dst[0].green = (src[0].green + src[1].green + src[dim].green + src[dim + 1].green) >> 2;
   dst[0].blue  = (src[0].blue + src[1].blue + src[dim].blue + src[dim + 1].blue) >> 2;

   // Right Top
   curr = dim - 1;
   dst[curr].red   = (src[curr].red + src[curr - 1].red + src[curr + dim - 1].red + src[curr + dim].red) >> 2;
   dst[curr].green = (src[curr].green + src[curr - 1].green + src[curr + dim - 1].green + src[curr + dim].green) >> 2;
   dst[curr].blue  = (src[curr].blue + src[curr - 1].blue + src[curr + dim - 1].blue + src[curr + dim].blue) >> 2;

   // Left Bottom
   curr *= dim;
   dst[curr].red   = (src[curr].red + src[curr + 1].red + src[curr - dim].red + src[curr - dim + 1].red) >> 2;
   dst[curr].green = (src[curr].green + src[curr + 1].green + src[curr - dim].green + src[curr - dim + 1].green) >> 2;
   dst[curr].blue  = (src[curr].blue + src[curr + 1].blue + src[curr - dim].blue + src[curr - dim + 1].blue) >> 2;

   // Right Bottom
   curr += dim - 1;
   dst[curr].red   = (src[curr].red + src[curr - 1].red + src[curr - dim].red + src[curr - dim - 1].red) >> 2;
   dst[curr].green = (src[curr].green + src[curr - 1].green + src[curr - dim].green + src[curr - dim - 1].green) >> 2;
   dst[curr].blue  = (src[curr].blue + src[curr - 1].blue + src[curr - dim].blue + src[curr - dim - 1].blue) >> 2;

   // Smooth four edges
   int ii, jj, limit;

   // Edge 0
   limit = dim - 1;
   for (ii = 1; ii < limit; ii++)
   {
     dst[ii].red   = (src[ii].red + src[ii - 1].red + src[ii + 1].red + src[ii + dim].red + src[ii + dim - 1].red + src[ii + dim + 1].red) / 6;
     dst[ii].green = (src[ii].green + src[ii - 1].green + src[ii + 1].green + src[ii + dim].green + src[ii + dim - 1].green + src[ii + dim + 1].green) / 6;
     dst[ii].blue  = (src[ii].blue + src[ii - 1].blue + src[ii + 1].blue + src[ii + dim].blue + src[ii + dim - 1].blue + src[ii + dim + 1].blue) / 6;
   }

   // Edge 3
   limit = dim * dim - 1;
   for (ii = (dim - 1) * dim + 1; ii < limit; ii++)
   {
     dst[ii].red   = (src[ii].red + src[ii - 1].red + src[ii + 1].red + src[ii - dim].red + src[ii - dim - 1].red + src[ii - dim + 1].red) / 6;
     dst[ii].green = (src[ii].green + src[ii - 1].green + src[ii + 1].green + src[ii - dim].green + src[ii - dim - 1].green + src[ii - dim + 1].green) / 6;
     dst[ii].blue  = (src[ii].blue + src[ii - 1].blue + src[ii + 1].blue + src[ii - dim].blue + src[ii - dim - 1].blue + src[ii - dim + 1].blue) / 6;
   }

   // Edge 1
   limit = dim * (dim - 1);
   for (jj = dim; jj < limit; jj += dim)
   {
     dst[jj].red = (src[jj].red + src[jj + 1].red + src[jj - dim].red + src[jj - dim + 1].red + src[jj + dim].red + src[jj + dim + 1].red) / 6;
     dst[jj].green = (src[jj].green + src[jj + 1].green + src[jj - dim].green+ src[jj - dim + 1].green + src[jj + dim].green + src[jj + dim + 1].green) / 6;
     dst[jj].blue = (src[jj].blue + src[jj + 1].blue + src[jj - dim].blue + src[jj - dim + 1].blue + src[jj + dim].blue + src[jj + dim + 1].blue) / 6;
   }

   // Edge 2
   for (jj = 2 * dim - 1 ; jj < limit ; jj += dim)
   {
     dst[jj].red = (src[jj].red + src[jj - 1].red + src[jj - dim].red + src[jj - dim - 1].red + src[jj + dim].red + src[jj + dim - 1].red) / 6;
     dst[jj].green = (src[jj].green + src[jj - 1].green + src[jj - dim].green + src[jj - dim - 1].green + src[jj + dim].green + src[jj + dim - 1].green) / 6;
     dst[jj].blue = (src[jj].blue + src[jj - 1].blue + src[jj - dim].blue + src[jj - dim - 1].blue + src[jj + dim].blue + src[jj + dim - 1].blue) / 6;
   }

   // Remaining pixels
   int i, j;
   for (i = 1 ; i < dim - 1 ; i++) {
     for (j = 1 ; j < dim - 1 ; j++) {
       curr = i * dim + j;
       dst[curr].red = (src[curr].red + src[curr - 1].red + src[curr + 1].red + src[curr - dim].red + src[curr - dim - 1].red + src[curr - dim + 1].red + src[curr + dim].red + src[curr + dim - 1].red + src[curr + dim + 1].red) / 9;
       dst[curr].green = (src[curr].green + src[curr - 1].green + src[curr + 1].green + src[curr - dim].green + src[curr - dim - 1].green + src[curr - dim + 1].green + src[curr + dim].green + src[curr + dim - 1].green + src[curr + dim + 1].green) / 9;
       dst[curr].blue = (src[curr].blue + src[curr - 1].blue + src[curr + 1].blue + src[curr - dim].blue + src[curr - dim - 1].blue + src[curr - dim + 1].blue + src[curr + dim].blue + src[curr + dim - 1].blue + src[curr + dim + 1].blue) / 9;
     }
   }

 }
char smooth_descr[] = "smooth: Current working version";
void smooth(int dim, pixel *src, pixel *dst) 
{
    // set_corner(0, src, dst, 1, dim, dim+1);
    // set_corner(dim-1, src, dst, dim-2, dim+dim-2, dim+dim-1);
    // set_corner(RIDX(dim-1, 0, dim), src, dst, RIDX(dim-1, 1, dim), RIDX(dim-2, 0, dim), RIDX(dim-2, 1, dim));
    // set_corner(RIDX(dim-1, dim-1, dim), src, dst, RIDX(dim-1, dim-2, dim), RIDX(dim-2, dim-2, dim), RIDX(dim-2, dim-1, dim));
    // // 处理四个边
    // for(int j = 1; j <= dim-2; j++){
    //     set_top(dim,  dst,src, j);
    //     set_bottom(dim,  dst, src,dim*dim-dim+j+1);
    //     set_left(dim,  dst,src, j*dim+1);
    //     set_right(dim,  dst,src, j*dim+dim);
    // }
    // // 中间部分
    // for(int i = 1; i <= dim-2; i++){
    //     for(int j = 1; j <= dim-2; j++){
    //         set_in(dim, src, dst, i*dim+j);
    //     }
    // }

    // int i, j;
    
    // for (i = 0; i < dim; i++)
	// for (j = 0; j < dim; j++)
	//     dst[RIDX(i, j, dim)] = avg(dim, i, j, src);     

    optimized_smooth(dim, src, dst);
}


/********************************************************************* 
 * register_smooth_functions - Register all of your different versions
 *     of the smooth kernel with the driver by calling the
 *     add_smooth_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_smooth_functions() {
    add_smooth_function(&smooth, smooth_descr);
    add_smooth_function(&naive_smooth, naive_smooth_descr);
    /* ... Register additional test functions here */
}

