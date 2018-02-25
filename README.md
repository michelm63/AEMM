# AEMM
Allumage Electronique Arduino michelm

Ce projet est un allumage électronique programmable cartographique pour moto ou automobile ancienne, il est développé pour utiliser un Arduino (Uno ou Nano V3).

ATTENTION il est toujours possible d'être modifié, amélioré, et si vous l'utilisez je dégage toute responsabilité de son utilisation, c'est à vos risques et périls je ne saurais être responsable des désagréments, incidents, accidents qu'il pourrait occasionner !!
Merci de votre compréhension, c'est un travail d'amateur pas d'une société commerciale.


Si vous voulez lire le déroulement de l'étude vous pouvez visiter le forum des Motos Anglaises d'avant 1983 le MAC :
http://motos-anglaises.com/phpBB3/viewtopic.php?f=3&t=28953

La réalisation de départ provient de l'AEPL, voir le site qui comporte beaucoup d'explications, de réalisations, de conseils :
http://a110a.free.fr/SPIP172/article.php3?id_article=142

L'AECP m'a aussi servi d'inspiration, sa réalisation est également très intéressante :
https://github.com/Totof34/aecp

l'AEMM par rapport à l'AEPL, comporte une avance non calculée à très faible régime moteur, l'allumage ne se fait qu'au PMH , pour éviter toute possibilité de retour de kick pour une moto.
Il n'y a pas d'allumage à la mise du contact, seulement si le moteur tourne.
Il offre la possibilité de modifier l'avance en roulant avec un commutateur.
Il permet l'allumage multi-étincelles jusqu'à environ 1500 tr/mn pour aider à un ralenti stable comme l'AEPL.
Il permet plusieurs courbes d'avance à choisir au démarrage (pas en roulant) comme l'AEPL.
Il permet une correction d'avance par dépression ou ouverture des gaz comme l'AECP.
Plusieurs courbes d'avance à dépression sont aussi possibles.
Il y a également une sortie pour une LED de contrôle de la charge batterie.

A noter que l'AEMM permet l'utilisation de bobine d'allumage moderne à faible résistance (bobine crayon auto par exemple), le dwell (temps de charge bobine) est variable en fonction de la tension batterie suivant une courbe type Bosch. 
Il convient de vérifier le temps de charge bobine suivant son utilisation. Il y a un risque de brûler une bobine d'allumage à très faible résistance primaire si le temps de charge bobine n'est pas adapté.
Pour une bobine "ancienne" ou à résistance élevée, de quelques ohms (3 ou 4)  on peut mettre un temps relativement long sans crainte. Cependant il est inutile de la saturer en permanence, sa durée de vie pourrait en être écourtée.
Il y a de nombreux commentaires avec le code pour aider à la compréhension.

Il y a aussi une version CDI, allumage à décharge de condensateur, cette version n'a pas besoin de gérer la charge des bobines ce qui simplifie le programme. Mais nécessite l'électronique de charge du condensateur d'allumage par exemple 2 uF à 300 V, la durée d'étincelle d'allumage étant plus courte il est utile de faire du multi-étincelles pour compenser, le circuit de charge du condensateur doit être le plus rapide possible.

Merci de m'avoir lu et bon bricolage !


The project describes a electronic mapped motorcycle or car ignition
especialy for two cylindre

If you would have more information (in french) about this project please visit 
http://motos-anglaises.com/phpBB3/viewtopic.php?f=3&t=28953


PCB & schema are designed with Kicad

the new board uses a Atmega328 and usb link to transmit some data to the PC
it's a Arduino Nano_V3

The idea comes from this site
http://a110a.free.fr/SPIP172/article.php3?id_article=142


for the rest, I suggest you to navigate into folders and open the pdf

The main code is explained through comments ( in French) in the ino file


Enjoy
