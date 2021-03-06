#include "skel.h"

int interfaces[ROUTER_NUM_INTERFACES];
struct route_table_entry *rtable;
int rtable_size;

struct arp_entry *arp_table;
int arp_table_len;

/*
 Returns a pointer (eg. &rtable[i]) to the best matching route
 for the given dest_ip. Or NULL if there is no matching route.
*/
struct route_table_entry *get_best_route(__u32 dest_ip) {
	
	int best_index = -1;
	for(int i = 0; i < rtable_size; i++){
		if((rtable[i].mask & dest_ip) == rtable[i].prefix){

			if((best_index == -1) || (ntohl(rtable[i].mask) > (ntohl(best_index))))
				best_index = i;
		}
	}

	if(best_index >= 0){
		return &rtable[best_index];
	}

	fprintf(stdout, "[Error]: best index not found\n");

	return NULL;
}

/*
 Returns a pointer (eg. &arp_table[i]) to the best matching ARP entry.
 for the given dest_ip or NULL if there is no matching entry.
*/
struct arp_entry *get_arp_entry(__u32 ip) {
    
    for(int i = 0; i < arp_table_len; i++){
    	if(arp_table[i].ip == ip){
    		return &(arp_table[i]);
    	}

    }
    fprintf(stdout, "[Error]: no matching entry\n");
    return NULL;
}

int main(int argc, char *argv[])
{
	msg m;
	int rc;

	init();
	rtable = malloc(sizeof(struct route_table_entry) * 100);
	arp_table = malloc(sizeof(struct  arp_entry) * 100);
	DIE(rtable == NULL, "memory");
	rtable_size = read_rtable(rtable);
	parse_arp_table();

	/* Students will write code here */
	while (1) {
		
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		
		/* TODO 3: Check the checksum */
		if(ip_checksum(ip_hdr, sizeof(struct iphdr)) != 0){
			printf("[Error]: Bad checksum\n");
			continue;
		}

		/* TODO 4: Check TTL >= 1 */
		if(ip_hdr->ttl < 1){
			fprintf(stdout, "[Error]: Bad TTL\n");
			continue;
		}


		/* TODO 5: Find best matching route (using the function you wrote at TODO 1) */
		struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);

		if(best_route == NULL){
			fprintf(stdout, "[Error]: Best route not found\n");
			continue;
		}
		
		/* TODO 6: Update TTL and recalculate the checksum */
		ip_hdr->ttl--;
		ip_hdr->check = 0;
		ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

		/* TODO 7: Find matching ARP entry and update Ethernet addresses */
		struct arp_entry *arp_entry = get_arp_entry(ip_hdr->daddr);
		get_interface_mac(best_route->interface, eth_hdr->ether_shost );
		memcpy(eth_hdr->ether_dhost, arp_entry->mac, sizeof(arp_entry->mac));

		/* TODO 8: Forward the pachet to best_route->interface */
		fprintf(stdout, "Sending packet on interface %d\n",
		best_route->interface);
		send_packet(best_route->interface, &m);
	}
}
