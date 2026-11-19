# Project Eldoria — Feuille de route « de la démo technique à un vrai morceau de jeu »

Constat honnête : la tranche actuelle prouve que les systèmes s'emboîtent (IA de Tharum, deux routes, libération, FORCE),
mais elle ne procure aucune sensation. La priorité a été mise sur les systèmes et les tests par téléportation ; le ressenti,
l'image, le son et la mise en scène ont été négligés. Ce document reprend la liste de défauts du joueur, la complète avec
ce qui fait qu'un Zelda moderne « tient », puis l'ordonne en jalons livrables (une PR par lot).

Légende : 🔴 bloquant pour l'immersion · 🟠 important · 🟢 finition

---

## 1. Diagnostic — la liste du joueur, développée

### Direction artistique (DA) — 🔴
« Zéro DA, je voulais BOTW/TOTK. » Ce qui fait la DA de BOTW/TOTK, concrètement :
- **Éclairage peint, pas réaliste** : ombrage cel-shadé doux (2-3 paliers, transitions douces), ombres teintées (bleu/violet, jamais noires), lumière chaude.
- **Perspective aérienne marquée** : le lointain se fond dans un brouillard bleuté clair ; c'est ce qui donne l'échelle et la profondeur.
- **Couleurs** : palette pastel saturée mais retenue, verts jaunes/olive, ciel très lumineux ; textures quasi absentes, remplacées par des aplats et des dégradés.
- **Formes** : silhouettes lisibles, gros volumes simples, détails rares mais bien placés ; herbe dense qui ondule, arbres en « boules » de feuillage.
- **Contours** : léger liseré sombre sur les silhouettes (rim/outline) et rim light clair.
- **Effets** : bloom doux, god rays, particules (pollen, lucioles, feuilles), vent visible.
- **Identité propre** : il faut aussi une culture visuelle à nous (les ruines de l'ancien peuple d'Eldoria, la technologie du mécanisme) — pas un copier-coller de Hyrule.

État actuel : rendu réaliste par défaut d'Unreal, assets Quaternius hétérogènes, greybox (aqueduc, escaliers, tour = cubes), eau = disque bleu.

### Le décor — 🔴
« Ça ressemble à rien, des carrés. » Tout le gameplay (aqueduc, escalier, tour, colonnade, niche du sanctuaire) est encore en blocs.
Il faut un **kit de ruines** modélisé (arches, piliers cannelés brisés, dalles, escaliers usés, végétation qui reprend les ruines)
et un vrai **point de vue d'arrivée** (le couloir doit s'ouvrir sur une vue cadrée de la clairière et de Tharum — le « moment Grand Plateau »).

### Tharum — 🔴
« On dirait un bébé. » Les proportions sont celles d'un éléphanteau : corps rond, pattes courtes, grosse tête.
Pour qu'il soit imposant : pattes longues et massives (colonnes), épaules hautes et dos qui descend, tête plus petite et basse,
défenses énormes et usées, peau craquelée comme de la pierre, mousse/plantes/ruines sur le dos (c'est un dieu ancien),
le mécanisme qui brille en contraste. Et surtout : **squelette + vraies animations** (aujourd'hui c'est une marionnette de pièces rigides).
Mise en scène de l'échelle : **chaque pas fait trembler la caméra et soulève de la poussière**, oiseaux qui s'envolent,
son grave, caméra qui le cadre en contre-plongée à la première rencontre.

### Le combat — 🔴
« Ça devrait t'éjecter à chaque verrou, 3 phases, difficulté croissante. »
- Chaque verrou arraché : Tharum se cabre et **éjecte le joueur** → retour au sol → nouvelle phase.
- **Phase 1** : charges + routine (il boit, il s'arrête) → les deux routes actuelles.
- **Phase 2** : il ne boit plus, **piétinement en onde de choc** (sauter pour l'éviter), **coup de trompe** circulaire, se méfie de la tour.
- **Phase 3** : enragé, plus rapide, **lance des rochers/troncs avec sa trompe**, fenêtres d'étourdissement plus courtes ; l'accès au dos demande un nouveau moyen (ex. utiliser ses propres rochers, l'escalade).
- **Lisibilité** : chaque attaque a un signal (posture, son, lueur du mécanisme), les points faibles brillent.
- **Enjeu** : le joueur doit pouvoir **prendre des dégâts et échouer** (cœurs, réapparition au point de contrôle).

### Physique et « goofy » — 🟠
« Les arbres qui volent, c'est moche et pas réaliste. » Les chutes doivent être **lourdes et lentes** (masse, amortissement, pas d'éjection),
les arbres doivent **plier puis basculer** (charnière au pied), les débris se poser. Remplacer les structures soudées par une vraie fracture
(Chaos/Geometry Collection) pour la tour.

### Collisions — 🔴
« On rentre dans tout. » Audit nécessaire : les rochers/arbres importés n'ont pas tous de collision, Tharum a des capsules qui ne collent pas
à son modèle, la végétation a été rendue traversable en bloc. Règle cible : **ce qui a l'air solide l'est** ; seuls l'herbe et les petites plantes sont traversables (et elles s'écartent au passage).

### Sensations du personnage — 🔴
- **Pas de skin, pas d'animations d'action** : le mannequin Unreal a marche/course/saut mais rien pour pousser, porter, lancer, interagir.
  → personnage stylisé avec un vrai design, animations : pousser (effort), porter, lancer, interagir, atterrissage lourd, glissade.
- **Pousser n'a aucun poids** : démarrage lent, posture d'effort, caméra qui se rapproche, vibrations/poussière/son de frottement, essoufflement.
- **Contrôleur** : accélération/décélération, virages, *coyote time*, saut tamponné, rattrapage de rebord (mantle), atterrissages.
- **Escalade + endurance** (ajout) : grimper presque partout est le cœur de BOTW ; la jauge d'endurance (roue verte) donne de la tension.

### Caméra et « juice » — 🔴
Ce qui manque et qu'un joueur ressent sans pouvoir le nommer :
- **Secousses de caméra** (pas de Tharum, impacts, chutes, éjection) avec intensité selon la distance.
- **Arrêt sur image (hit-stop)** et ralenti bref sur les moments forts (verrou arraché, libération).
- **Retour de FOV** : léger zoom arrière au sprint, avant à l'effort.
- **Caméra cinématique** : présentation du boss, cadrage de la libération ; **verrouillage de cible** (Z-targeting) sur Tharum/le mécanisme.
- **Post-process** : profondeur de champ en cinématique, flou de mouvement léger sur la caméra uniquement, vignettage dynamique, aberration chromatique ponctuelle à l'impact.
- **Particules** : poussière, éclats, étincelles du mécanisme, feuilles, éclaboussures.
- **Retour manette** : vibrations.

### Textes et narration — 🟠
« Hyper verbeux, des textes inutiles ; certains devraient être dits par un allié/une fée. »
- Couper ~70 % des messages ; l'environnement doit raconter (traces de pas géantes, ruines, fresques).
- **Compagnon** : un petit esprit/une luciole d'Eldoria qui commente rarement, avec un nom, une bulle de dialogue et un son « blabla » à la Zelda (en attendant de vraies voix).
- Le HUD n'affiche que l'essentiel ; la narration passe par la mise en scène.

### Interface et typographie — 🟠
« Aucune DA pour les polices. » Polices libres (OFL) : une police de titre gravée/antique pour les noms (boss, lieux),
une police ronde et lisible pour les dialogues/invites. HUD dessiné : cœurs, roue d'endurance, invites avec icône de touche (AZERTY/manette),
barre de boss ornementée, cartouches de dialogue du compagnon, écran titre et menu pause.

### Son — 🔴 (absent)
- **Ambiance** : jungle (insectes, oiseaux, vent, eau), qui se tait quand Tharum approche.
- **Bruitages** : pas selon la surface, pousser/frotter, pierre qui casse, mécanisme (cliquetis, bourdonnement du noyau), pas de Tharum (infrabasses), barrissement.
- **Musique** : exploration minimaliste (piano/notes éparses, comme BOTW), musique de boss en couches selon la phase, jingle de libération/de capacité.

### Eau, végétation, vie — 🟢
Vraie eau (shader stylisé, reflets, mousse au bord), herbe qui s'écarte, petites créatures (oiseaux, papillons), lucioles au crépuscule.

### Technique — 🟢
Performances (budget fps), menu des options (sensibilité, inversion, remappage), sauvegarde/points de contrôle, écran titre.

---

## 2. Jalons (une PR par jalon, chacun testable)

| # | Jalon | Contenu | Pourquoi dans cet ordre |
|---|-------|---------|-------------------------|
| **J1** | Bible de DA + rendu | Planche de style (palette, références, règles), post-process cel-shading + contours, ciel/brouillard/lumière, matériaux maîtres stylisés, polices et charte UI | Tout ce qu'on modélise ensuite dépend de ce rendu |
| **J2** | Solidité du monde | Audit et génération des collisions, végétation traversable seulement si petite, chutes lourdes (arbres à charnière, tour en fracture), plus d'objets qui volent | Corrige « on rentre dans tout » / « goofy » |
| **J3** | Kit de ruines + décor | Modèles Blender (aqueduc, escaliers, tour, colonnade, sanctuaire) dans la DA, eau stylisée, vue d'arrivée cadrée | Supprime le greybox |
| **J4** | Personnage | Skin stylisé, animations d'action (pousser, porter, lancer, interagir), contrôleur, escalade + endurance, cœurs/dégâts/réapparition | Le joueur doit se sentir bien avant le boss |
| **J5** | Caméra et juice | Secousses, hit-stop, FOV, verrouillage de cible, cinématiques courtes, particules, vibrations | Donne du poids à tout le reste |
| **J6** | Tharum v2 | Nouveau modèle imposant, squelette + animations, pas qui font trembler, combat en 3 phases avec éjection, nouvelles attaques, signaux | Le gros morceau, sur des bases saines |
| **J7** | Son | Ambiances, bruitages, musique en couches | Peut avancer en parallèle de J4-J6 |
| **J8** | Narration | Réécriture des textes, compagnon-esprit, écran titre/pause | Finition |

---

## 3. Ressources et limites (à valider)
- **Modèles** : modélisation procédurale dans Blender (gratuit, contrôle total, mais limité en finesse) ; génération 3D par IA via le module Blender
  (rapide, plus organique, **coûte des crédits**) ; packs CC0 (Quaternius, Kenney, Poly Haven). Rien sous licence Nintendo.
- **Animations** : animations CC0 (ex. Quaternius Universal Animation Library) reciblées, ou animations créées par script dans Blender.
- **Sons** : CC0 (Freesound filtré CC0, Kenney, Sonniss GDC bundle libre de droits) ; musique : CC0/libre ou composition procédurale simple.
- **Polices** : Google Fonts (licence OFL).
- Chaque téléchargement de ressource est soumis à accord préalable.
