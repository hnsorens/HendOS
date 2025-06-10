/* kmath.c */
#include <kmath.h>

/**
 * @brief Absolute value of an integer (float precision)
 */
float fabs(float x)
{
    return x < 0.0 ? -x : x;
}

/**
 * @brief Absolute value of an integer
 */
int32_t abs(int32_t x)
{
    return x < 0 ? -x : x;
}

/**
 * @brief Minimum of two integers
 */
int32_t min(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

/**
 * @brief Maximum of two integers
 */
int32_t max(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

/**
 * @brief Clamp a float value between min and max
 */
float clampf(float x, float min, float max)
{
    return x < min ? min : (x > max ? max : x);
}

/**
 * @brief Clamp an integer value between min and max
 */
int32_t clamp(int32_t x, int32_t min, int32_t max)
{
    return x < min ? min : (x > max ? max : x);
}

/**
 * @brief Sine frunction (float precision)
 */
float sinf(float x)
{
    x = x - (int)(x / 6.283185f) * 6.283185f;
    float xx = x * x;
    return x * (1.0f - xx / 6.0f * (1.0f - xx / 20.0f * (1.0f - xx / 42.0f)));
}

/**
 * @brief Sine function (double precision)
 */
float sin(float x)
{
    x = x - (int)(x / 6.283185) * 6.283185;
    double xx = x * x;
    return x * (1.0f - xx / 6.0 * (1.0 - xx / 20.0 * (1.0 - xx / 42.0)));
}

/**
 * @brief Cosine function (float precision)
 */
double cosf(double x)
{
    return sinf(x + 1.570796f);
}

/**
 * @brief Cosine function (double precision)
 */
double cos(double x)
{
    return sin(x + 1.570796f);
}

/**
 * @brief Square root (float precision)
 */
float sqrtf(float x)
{
    if (x < 0)
    {
        return 0;
    }
    float result = x;
    float epsilon = 0.00001;
    while (result * result > x)
    {
        result = (result + x / result) / 2.0;
    }
    return result;
}

/**
 * @brief Square root (double precision)
 */
double sqrt(float x)
{
    if (x < 0)
    {
        return 0;
    }
    double result = x;
    double epsilon = 0.00001;
    while (result * result > x)
    {
        result = (result + x / result) / 2.0;
    }
    return result;
}

/**
 * @brief Floor function
 */
double floor(double x)
{
    int i = (int)x;
    return (x < i) ? i - 1 : i;
}

/**
 * @brief Ceiling function
 */
double ceiling(double x)
{
    int i = (int)x;
    return (x > i) ? i + 1 : i;
}

/**
 * @brief Power function (integer exponents only)
 */
double pow(double base, uint32_t exp)
{
    double result = 1;
    for (int i = 0; i < exp; i++)
    {
        result *= base;
    }
    return result;
}

/**
 * @brief Floating-point remainder
 */
double fmod(double x, double y)
{
    if (y == 0.0)
        return 0.0; // undefined, return 0 for safety
    int quotient = (int)(x / y);
    return x - (double)quotient * y;
}

/**
 * @brief Arccosine function
 */
double acos(double x)
{
    if (x < -1.0 || x > 1.0)
        return 0.0;
    double x2 = x * x;
    double term = x;
    double result = x;
    double fact_num = 1.0;
    double fact_den = 1.0;
    for (int n = 1; n < 10; n++)
    {
        fact_num *= (2.0 * n - 1);
        fact_den *= (2.0 * n);
        term *= x2;
        double frac = (fact_num / fact_den) * term / (2.0 * n + 1.0);
        result += frac;
    }
    double pi = 3.141592653589793;
    return (pi / 2.0) - result;
}

/**
 * @brief Assertion macro implementation
 */
void __imp__assert(int i)
{
    if (!(i))
    {
        // Print(L"Assertion failed: %s, file %s, line %d\n", i, __FILE__, __LINE__);
    }
}