# Meldingstjeneste i C

### OM
Programmet er en del av hjemmeeksamen 2022 i IN2140.

### Kjøre programet
Programmet må kjøres i linux terminalen. Compiler alle filene med "make all" inni mappen til prosjektet. Sett dermed opp serveren med "make runserver" eller manuelt med "./upush_server SERVER-PORT TAPSSANSYNLIGHET". Dermed kan klienter opprettes. Der er lagt til to klienter med "make runclient" og "make runclient2" som kan brukes. Evuntulelt kan flere legges til med ./upush_client KLIENTNAVN IP-ADRESSE PORTNUMMER TIMOUT TAPSSANSYNLIGHET

ip adresse er den lokale adressen 127.0.0.1
protnummer for klient må samsvare for portnummer for server. Standard er 2001
timout er tiden klienten venter før nytt forsøk om en pakke går tap i sek. Velg feks 5
tapssansynlighet er sannsynligheten for at en pakke går tapt i prosent
