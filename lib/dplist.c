//
// Created by sodir on 12/25/24.
//

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"
#include <stdbool.h>

/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;

    void *(*element_copy)(void *src_element);

    void (*element_free)(void **element);

    int (*element_compare)(void *x, void *y);
};


dplist_t *dpl_create(// callback functions
        void *(*element_copy)(void *src_element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {
    if(*list == NULL) {
      return;
    }

    int size = dpl_size(*list);
    if(size == 0){
        free(*list);
        *list = NULL;
        return;
    }

    for (int index = size - 1; index >= 0; index--) {
        *list = dpl_remove_at_index(*list, index, free_element);
    }
    free(*list);
	*list = NULL;
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {
    dplist_node_t *ref_at_index, *list_node;
    if (list == NULL){
      return NULL;
	}
    list_node = malloc(sizeof(dplist_node_t));
    //idem als ervoor, if else doen
    if(insert_copy) {
      list_node->element = list->element_copy(element);
      } else {
        list_node->element = element;
  	}
    // pointer drawing breakpoint
    if (list->head == NULL) { // covers case 1
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
        // pointer drawing breakpoint
    } else if (index <= 0) { // covers case 2
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
        // pointer drawing breakpoint
    } else {
        ref_at_index = dpl_get_reference_at_index(list, index);
        assert(ref_at_index != NULL);
        // pointer drawing breakpoint
        if (index < dpl_size(list)) { // covers case 4
            list_node->prev = ref_at_index->prev;
            list_node->next = ref_at_index;
            ref_at_index->prev->next = list_node;
            ref_at_index->prev = list_node;
            // pointer drawing breakpoint
        } else { // covers case 3
            assert(ref_at_index->next == NULL);
            list_node->next = NULL;
            list_node->prev = ref_at_index;
            ref_at_index->next = list_node;
            // pointer drawing breakpoint
        }
    }
    return list;
}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {
    if (list == NULL) return NULL;
    if (list->head == NULL) return list;

    dplist_node_t *remove_node = dpl_get_reference_at_index(list, index);
    if (remove_node == NULL) return list;

    // If removing head node
    if (remove_node == list->head) {
        list->head = remove_node->next;
        if (list->head != NULL) {
            list->head->prev = NULL;
        }
    } else {
        // Remove from middle/end
        if (remove_node->prev != NULL) {
            remove_node->prev->next = remove_node->next;
        }
        if (remove_node->next != NULL) {
            remove_node->next->prev = remove_node->prev;
        }
    }

    if (free_element) {
        list->element_free(&remove_node->element);
    }
    free(remove_node);
    return list;
}

int dpl_size(dplist_t *list) {
    if (list == NULL) {
        return -1;
    }

    if (list->head == NULL) {
        return 0;
    }

    int counter = 0;
    dplist_node_t *currNode = list->head;
    while (currNode != NULL) {
        counter++;
        currNode = currNode->next;
    }
    return counter;

}

void *dpl_get_element_at_index(dplist_t *list, int index) {
    if (list == NULL) {
      return NULL;
    }

    int size = dpl_size(list);

    if(size == 0) {
        return NULL;
    }

    if(index <=0 ) {
        return list->head->element;
    }

    if(index >= size) {
        dplist_node_t *dummy = list->head;
        while (dummy->next != NULL) {
            dummy = dummy->next;
        }
        return dummy->element;
    }

    dplist_node_t *dummy = list->head;
    int count = 0;
    while (count < index) {
        dummy = dummy->next;
        count++;
    }
    return dummy->element;
}

int dpl_get_index_of_element(dplist_t *list, void *element) {
    if (list == NULL) {
        return -1;
    }

    int index = 0;
    dplist_node_t *currentNode = list->head;

    while (currentNode != NULL) {
        void *currElement = currentNode->element;
        if (list->element_compare(currElement, element) == 0) {
            return index;
        }
        currentNode = currentNode->next;
        index++;
    }
    return -1;
}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
    if (list == NULL) {
        return NULL;
    }
    int size = dpl_size(list);
    dplist_node_t *dummy = list->head;

    if (size == 0) {
      return NULL;
    }

    if(index <=0) {
        return list -> head;
    }

    if (index >= size) {
        while (dummy->next != NULL) {
            dummy = dummy->next;
        }
        return dummy;
    }

    int count = 0;
    while (count < index) {
        dummy = dummy->next;
        count++;
    }
    return dummy;
}



void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {
    if (list == NULL){
      return NULL;
    }

    int size = dpl_size(list);

    if(size == 0) {
        return NULL;
    }

    dplist_node_t *dummy = list->head;
    while (dummy != NULL) {
        if (dummy == reference) {
            return dummy->element;
        }
        dummy = dummy->next;
    }
    return NULL;
}

