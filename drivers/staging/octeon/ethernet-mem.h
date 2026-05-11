/*
 * This file is based on code from OCTEON SDK by Cavium Networks.
 *
 * Copyright (c) 2003-2007 Cavium Networks
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, as
 * published by the Free Software Foundation.
 */

int cvm_oct_mem_fill_fpa(int pool, int size, int elements);
struct platform_device;

void cvm_oct_mem_empty_fpa(struct platform_device *pdev, int pool, int size,
			   int elements);
