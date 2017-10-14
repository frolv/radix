/*
 * kernel/irq/interrupts.c
 * Copyright (C) 2017 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <radix/error.h>
#include <radix/irq.h>
#include <radix/slab.h>

/*
 * request_irq:
 * Request an IRQ for the specified globally unique device, to be handled
 * by the specified handler function. Returns the IRQ number if successful,
 * or a negative number indicating the error if not.
 */
int request_irq(void *device, irq_handler_t handler, unsigned long flags)
{
	struct irq_descriptor *desc;
	int irq;

	if (!device || !handler)
		return -EINVAL;

	desc = kmalloc(sizeof *desc);
	if (!desc)
		return -ENOMEM;

	desc->handler = handler;
	desc->device = device;
	desc->flags = flags;
	desc->next = NULL;

	irq = __arch_request_irq(desc);
	if (irq < 0)
		kfree(desc);

	return irq;
}

/*
 * request_specific_irq:
 * Request to register a specific IRQ number for the specified globally unique
 * device. To be used for devices which are wired to a specific interrupt pin.
 * Returns 0 on success, error code on failure.
 */
int request_fixed_irq(unsigned int irq, void *device, irq_handler_t handler)
{
	if (!device || !handler)
		return EINVAL;

	return __arch_request_fixed_irq(irq, device, handler);
}

/*
 * release_irq:
 * Indicate that the specified IRQ is no longer being used by the given device.
 */
void release_irq(unsigned int irq, void *device)
{
	__arch_release_irq(irq, device);
}
