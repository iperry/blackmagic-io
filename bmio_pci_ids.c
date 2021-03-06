/* -LICENSE-START-
** Copyright (c) 2013 Blackmagic Design
**
** Permission is hereby granted, free of charge, to any person or organization
** obtaining a copy of the software and accompanying documentation covered by
** this license (the "Software") to use, reproduce, display, distribute,
** execute, and transmit the Software, and to prepare derivative works of the
** Software, and to permit third-parties to whom the Software is furnished to
** do so, all subject to the following:
** 
** The copyright notices in the Software and this entire statement, including
** the above license grant, this restriction and the following disclaimer,
** must be included in all copies of the Software, in whole or in part, and
** all derivative works of the Software, unless such copies or derivative
** works are solely in the form of machine-executable object code generated by
** a source language processor.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
** -LICENSE-END-
*/
#include <linux/pci.h>
#include <linux/module.h>

struct pci_device_id bmio_pci_ids[] =
{
	{ PCI_DEVICE(0xbdbd, 0xa12d) }, // UltraStudio 4K
	{ PCI_DEVICE(0xbdbd, 0xa12e) }, // DeckLink 4K Extreme
	{ PCI_DEVICE(0xbdbd, 0xa130) }, // DeckLink Mini Recorder
	{ PCI_DEVICE(0xbdbd, 0xa132) }, // UltraStudio 4K
	{ PCI_DEVICE(0xbdbd, 0xa136) }, // DeckLink 4K Extreme 12G
	{ PCI_DEVICE(0xbdbd, 0xa137) }, // DeckLink Studio 4K
	{ PCI_DEVICE(0xbdbd, 0xa138) }, // DeckLink SDI 4K
	{ PCI_DEVICE(0xbdbd, 0xa139) }, // Intensity Pro 4K
	{ PCI_DEVICE(0xbdbd, 0xa13b) }, // DeckLink Micro Recorder
	{ PCI_DEVICE(0xbdbd, 0xa13c) }, // Avid Artist DNxIO
	{ PCI_DEVICE(0xbdbd, 0xa13d) }, // DeckLink 4K Pro
	{ PCI_DEVICE(0xbdbd, 0xa13e) }, // UltraStudio 4K Extreme
	{ 0 }
};

MODULE_DEVICE_TABLE(pci, bmio_pci_ids);
