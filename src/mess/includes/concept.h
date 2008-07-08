/*****************************************************************************
 *
 * includes/concept.h
 *
 * Corvus Concept driver
 *
 * Raphael Nabet, 2003
 *
 ****************************************************************************/

#ifndef CONCEPT_H_
#define CONCEPT_H_


/*----------- defined in machine/concept.c -----------*/

MACHINE_START(concept);
VIDEO_START(concept);
VIDEO_UPDATE(concept);
INTERRUPT_GEN( concept_interrupt );
READ16_HANDLER(concept_io_r);
WRITE16_HANDLER(concept_io_w);


#endif /* CONCEPT_H_ */
