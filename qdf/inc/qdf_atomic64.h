/*
 * Copyright (c) 2014-2020 The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 *
 */

/**
 * DOC: qdf_atomic64.h
 * This file provides OS abstraction for atomic64 APIs.
 */

#ifndef _QDF_ATOMIC64_H
#define _QDF_ATOMIC64_H

#include <i_qdf_atomic64.h>

/**
 * typedef qdf_atomic64_t - atomic64 type of variable
 *
 * Use this when you want a simple resource counter etc. which is atomic
 * across multiple CPU's. These maybe slower than usual counters on some
 * platforms/OS'es, so use them with caution.
 */
typedef __qdf_atomic64_t qdf_atomic64_t;

/**
 * qdf_atomic64_init() - initialize an atomic64 type variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: QDF_STATUS
 */
static inline QDF_STATUS qdf_atomic64_init(qdf_atomic64_t *v)
{
	return __qdf_atomic64_init(v);
}

/**
 * qdf_atomic64_read() - read the value of an atomic64 variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: The current value of the variable
 */
static inline int64_t qdf_atomic64_read(qdf_atomic64_t *v)
{
	return __qdf_atomic64_read(v);
}

/**
 * qdf_atomic64_inc() - increment the value of an atomic64 variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: None
 */
static inline void qdf_atomic64_inc(qdf_atomic64_t *v)
{
	__qdf_atomic64_inc(v);
}

/**
 * qdf_atomic64_dec() - decrement the value of an atomic64 variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: None
 */
static inline void qdf_atomic64_dec(qdf_atomic64_t *v)
{
	__qdf_atomic64_dec(v);
}

/**
 * qdf_atomic64_add() - add a value to the value of an atomic64 variable
 * @i: The amount by which to increase the atomic64 counter
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: None
 */
static inline void qdf_atomic64_add(int64_t i, qdf_atomic64_t *v)
{
	__qdf_atomic64_add(i, v);
}

/**
 * qdf_atomic64_sub() - Subtract a value from an atomic64 variable
 * @i: the amount by which to decrease the atomic64 counter
 * @v: a pointer to an opaque atomic64 variable
 *
 * Return: none
 */
static inline void qdf_atomic64_sub(int64_t i, qdf_atomic64_t *v)
{
	__qdf_atomic64_sub(i, v);
}

/**
 * qdf_atomic64_dec_and_test() - decrement an atomic64 variable and check if
 * the new value is zero
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return:
 * true (non-zero) if the new value is zero,
 * false (0) if the new value is non-zero
 */
static inline int qdf_atomic64_dec_and_test(qdf_atomic64_t *v)
{
	return __qdf_atomic64_dec_and_test(v);
}

/**
 * qdf_atomic64_set() - set a value to the value of an atomic64 variable
 * @v: A pointer to an opaque atomic64 variable
 * @i: required value to set
 *
 * Atomically sets the value of v to i
 * Return: None
 */
static inline void qdf_atomic64_set(qdf_atomic64_t *v, int64_t i)
{
	__qdf_atomic64_set(v, i);
}

/**
 * qdf_atomic64_inc_return() - return the incremented value of an atomic64
 * variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: The current value of the variable
 */
static inline int64_t qdf_atomic64_inc_return(qdf_atomic64_t *v)
{
	return __qdf_atomic64_inc_return(v);
}

/**
 * qdf_atomic64_dec_return() - return the decremented value of an atomic64
 * variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: The current value of the variable
 */
static inline int64_t qdf_atomic64_dec_return(qdf_atomic64_t *v)
{
	return __qdf_atomic64_dec_return(v);
}

/**
 * qdf_atomic64_inc_not_zero() - increment if not zero
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: Returns non-zero on successful increment and zero otherwise
 */
static inline int64_t qdf_atomic64_inc_not_zero(qdf_atomic64_t *v)
{
	return __qdf_atomic64_inc_not_zero(v);
}

#endif
