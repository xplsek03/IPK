void send_syn()
{
	struct ip ip;
	struct tcphdr tcp;
	const int on = 1;
	struct sockaddr_in sin;

	u_char *packet;
	packet = (u_char *)malloc(60);
	

	ip.ip_hl = 0x5;
	ip.ip_v = 0x4;
	ip.ip_tos = 0x0;
	ip.ip_len = sizeof(struct ip) + sizeof(struct tcphdr); 
	ip.ip_id = htons(12830);
	ip.ip_off = 0x0;
	ip.ip_ttl = 64;
	ip.ip_p = IPPROTO_TCP;
	ip.ip_sum = 0x0;
	ip.ip_src.s_addr = inet_addr("172.17.14.90");
	ip.ip_dst.s_addr = inet_addr("172.16.1.204");
	ip.ip_sum = in_cksum((unsigned short *)&ip, sizeof(ip));
	memcpy(packet, &ip, sizeof(ip));

	tcp.th_sport = htons(3333);
	tcp.th_dport = htons(33334);
	tcp.th_seq = htonl(0x131123);
	tcp.th_off = sizeof(struct tcphdr) / 4;
	tcp.th_flags = TH_SYN;
	tcp.th_win = htons(32768);
	tcp.th_sum = 0;
	tcp.th_sum = in_cksum_tcp(ip.ip_src.s_addr, ip.ip_dst.s_addr, (unsigned short *)&tcp, sizeof(tcp));
	memcpy((packet + sizeof(ip)), &tcp, sizeof(tcp));
	
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ip.ip_dst.s_addr;

	if (sendto(sd, packet, 60, 0, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0)  {
		perror("sendto");
		exit(1);
	}
}


void *run(void *arg)
{
	struct ip ip;
	struct tcphdr tcp;
	const int on = 1;
	struct sockaddr_in sin;


	if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("raw socket");
		exit(1);
	}

	if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	send_syn(sd);

	
}





void raw_packet_receiver(u_char *udata, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
	struct ip *ip;
	struct tcphdr *tcp;
	u_char *ptr;
	int l1_len = (int)udata;
	int s_seq;

	ip = (struct ip *)(packet + l1_len);
	tcp = (struct tcphdr *)(packet + l1_len + sizeof(struct ip));

	printf("%d\n", l1_len);

	printf("a packet came, ack is: %d\n", ntohl(tcp->th_ack));
	printf("a packet came, seq is: %u\n", ntohl(tcp->th_seq));
	s_seq = ntohl(tcp->th_seq);

	// FINISH IT

	sleep(100);
}




void *pth_capture_run(void *arg)
{
	pcap_t *pd;
	char *filter = "dst host 172.17.14.90 and ip";
	char *dev = "fxp0";
	char errbuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32	netp;
	bpf_u_int32	maskp;
	struct bpf_program	fprog;					/* Filter Program	*/
	int dl = 0, dl_len = 0;

	if ((pd = pcap_open_live(dev, 1514, 1, 500, errbuf)) == NULL) {
		fprintf(stderr, "cannot open device %s: %s\n", dev, errbuf);
		exit(1);
	}

	pcap_lookupnet(dev, &netp, &maskp, errbuf);
	pcap_compile(pd, &fprog, filter, 0, netp);
	if (pcap_setfilter(pd, &fprog) == -1) {
		fprintf(stderr, "cannot set pcap filter %s: %s\n", filter, errbuf);
		exit(1);
	}
	pcap_freecode(&fprog);
	dl = pcap_datalink(pd);
	
	switch(dl) {
		case 1:
			dl_len = 14;
			break;
		default:
			dl_len = 14;
			break;
	}

	if (pcap_loop(pd, -1, raw_packet_receiver, (u_char *)dl_len) < 0) {
		fprintf(stderr, "cannot get raw packet: %s\n", pcap_geterr(pd));
		exit(1);
	}
}




int main(int argc, char **argv)
{
	pthread_t tid_pr;

	if (pthread_create(&tid_pr, NULL, pth_capture_run, NULL) != 0) {
		fprintf(stderr, "cannot create raw packet reader: %s\n", strerror(errno));
		exit(1);
	}
	printf("raw packet reader created, waiting 1 seconds for packet reader thread to settle down...\n");
	sleep(1);

	run(NULL);	

	pthread_join(tid_pr, NULL);
	return 0;
}