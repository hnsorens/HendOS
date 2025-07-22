/**
 * @file kmath.h
 * @brief Kernel Math Utility Interface
 *
 * Provides basic mathematical operations and macros for integer and floating-point math.
 */
#ifndef K_MATH_H
#define K_MATH_H

#define ALIGN_DOWN(addr, align) ((addr) & ~((align)-1))
#define ALIGN_UP(addr, align) (((addr) + (align)-1) & ~((align)-1))
#define MAX(v1, v2) ((v1) > (v2) ? (v1) : (v2))
#define MIN(v1, v2) ((v1) < (v2) ? (v1) : (v2))

#include <kint.h>

/**
 * @brief Absolute value of an integer (float precision)
 * @param x Input value
 * @return Absolute value of x
 */
__attribute__((unused)) float fabs(float x);

/**
 * @brief Absolute value of an integer
 * @param x Input value
 * @return Absolute value of x
 */
__attribute__((unused)) int32_t abs(int32_t x);

/**
 * @brief Minimum of two integers
 * @param a First value
 * @param b Second value
 * @return The smaller of a or b
 */
__attribute__((unused)) int32_t min(int32_t a, int32_t b);

/**
 * @brief Maximum of two integers
 * @param a First value
 * @param b Second value
 * @return The smaller of a or b
 */
__attribute__((unused)) int32_t max(int32_t a, int32_t b);

/**
 * @brief Clamp a float value between min and max
 * @param x Value to clamp
 * @param min Minimum bound
 * @param max Maximum bound
 * @return Clamped value between min and max
 */
__attribute__((unused)) float clampf(float x, float min, float max);

/**
 * @brief Clamp an integer value between min and max
 * @param x Value to clamp
 * @param min Minimum bound
 * @param max Maximum bound
 * @return Clamped value between min and max
 */
__attribute__((unused)) int32_t clamp(int32_t x, int32_t min, int32_t max);

/**
 * @brief Sine frunction (float precision)
 * @param x Angle in radians
 * @return Sine of x
 */
__attribute__((unused)) float sinf(float x);

/**
 * @brief Sine function (double precision)
 * @param x Angle in radians
 * @return Sine of x
 */
__attribute__((unused)) float sin(float x);

/**
 * @brief Cosine function (float precision)
 * @param return x Angle in radians
 * @return Cosine of x
 */
__attribute__((unused)) double cosf(double x);

/**
 * @brief Cosine function (double precision)
 * @param return x Angle in radians
 * @return Cosine of x
 */
__attribute__((unused)) double cos(double x);

/**
 * @brief Square root (float precision)
 * @param x Non-negative value
 * @return Square root of x, or 0 for negative inputs
 */
__attribute__((unused)) float sqrtf(float x);

/**
 * @brief Square root (double precision)
 * @param x Non-negative value
 * @return Square root of x, or 0 for negative inputs
 */
__attribute__((unused)) double sqrt(float x);

/**
 * @brief Floor function
 * @param x Input value
 * @return Largest integer not greater than x
 */
__attribute__((unused)) double floor(double x);

/**
 * @brief Ceiling function
 * @param x Input value
 * @return Smallest integer not less than x
 */
__attribute__((unused)) double ceiling(double x);

/**
 * @brief Power function (integer exponents only)
 * @param base Base value
 * @param exp Non-negative integer exponent
 * @return base raised to the power of exp
 */
__attribute__((unused)) double pow(double base, uint32_t exp);

/**
 * @brief Floating-point remainder
 * @param x Numerator
 * @param y Denominator
 * @return Remainder of x/y, or 0 if y is 0
 */
__attribute__((unused)) double fmod(double x, double y);

/**
 * @brief Arccosine function
 * @param x Value in range [-1, 1]
 * @return Arccosine in radians or 0 for invalid inputs
 */
__attribute__((unused)) double acos(double x);

/**
 * @brief Assertion macro implementation
 * @param i Condition to verify
 * @note Does nothing in release builds
 */
__attribute__((unused)) void __imp__assert(int i);

#endif /* K_MATH_H */