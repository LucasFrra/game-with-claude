# Bible de direction artistique — Eldoria

Référence d'esprit : *Breath of the Wild / Tears of the Kingdom* — un monde **peint, lumineux, lisible**, où la distance
et la lumière racontent l'échelle. Pas une copie d'Hyrule : Eldoria a sa propre culture (un peuple ancien de bâtisseurs,
leurs ruines de pierre claire, et une technologie de laiton et de lumière turquoise qui asservit les divinités).

## 1. Règles de rendu
| Règle | Concrètement | Où c'est réglé |
|-------|--------------|----------------|
| Lumière peinte | Ombrage en 3 paliers doux (ombre teintée / éclairé / reflet), transitions douces | `M_PP_EldoriaToon` (post-process, `art_direction.py`) |
| Ombres jamais noires | Ombres bleu-lavande `(0.70, 0.74, 0.92)`, niveau 0.58 | paramètres `ShadowTint`, `ShadowLevel` |
| Perspective aérienne | Brouillard bleu ciel pâle dès 15 m, opacité max 0.85 ; le lointain se fond | `ExponentialHeightFog` |
| Contours | Liseré fin et sombre (pas noir) sur les silhouettes proches, disparaît au loin | `Outline*` |
| Couleurs | Saturation retenue (1.04), contraste doux (0.94), hautes lumières chaudes, ombres froides | `PostProcessVolume` |
| Peu de texture | Aplats et dégradés plutôt que du bruit ; détails rares mais choisis | matériaux maîtres |

## 1b. Ce qui fait vraiment l'image (leçons du J1b)
Le filtre d'ombrage seul ne suffit pas. L'essentiel vient de :
- **Ciel et lointain** : nuages bas et épais (couche 1,5–4,5 km), ciel bleu non voilé (brouillard coupé à 5 km),
  sommets enneigés à 2–3,5 km qui dépassent des falaises (`SM_Peak_*`, `M_Eldoria_Distant`).
- **Herbe** : touffes de brins fins (`SM_Grass_Clump_A/B`) dont **toutes les normales pointent vers le haut** → éclairées comme le sol,
  couleur de racine = couleur du sol, pointes jaunies ; ~3× plus dense, disparaît après 45 m.
- **Arbres** : couronnes rondes (cœur opaque + cartes de feuilles `T_LeafCluster`) aux **normales sphériques** depuis le centre de la couronne ;
  c'est ce qui donne la séparation lumière/ombre douce des arbres de BOTW. Généré par `blender_botw_assets.py` (Blender sans interface).
- **Rendu léger** : pas de Lumen ni de ray tracing (inutile pour ce style et trop lourd pour 4 Go de VRAM) ; ombres classiques, AO écran, TAA.
- Toujours juger le rendu en qualité **Epic** (`scalability 3`), jamais sur une capture en qualité réduite.

## 2. Palette (couleurs de base, linéaire)
| Élément | Clair | Sombre |
|---------|-------|--------|
| Herbe / sol | `0.27 0.42 0.07` | `0.15 0.28 0.045` |
| Mousse | `0.13 0.27 0.05` | — |
| Terre / chemin | `0.55 0.36 0.16` | `0.40 0.25 0.10` |
| Roche (falaises) | `0.40 0.35 0.27` | `0.27 0.23 0.18` |
| Peau de Tharum (pierre ardoise) | `0.30 0.30 0.31` | `0.16 0.165 0.18` |
| Mécanisme (laiton) / lumière | laiton patiné | turquoise `0.12 0.75 0.62` |
| Interface : crème / or / encre | `1 0.955 0.86` / `0.98 0.80 0.42` | `0.09 0.08 0.07` |

Règle d'or de l'herbe : **la couleur des brins = la couleur du sol** sous eux, pour que le tapis paraisse dense et uni.

## 3. Formes
- Silhouettes lisibles à 50 m : gros volumes simples, une seule idée par objet.
- Ruines d'Eldoria : pierre claire, arêtes émoussées, arches et colonnes épaisses, gravures en spirale, végétation qui reprend le dessus.
- Échelle : le joueur ~1,8 m, Tharum ~14 m au dos, l'aqueduc 11 m — tout doit se lire par rapport au joueur.

## 4. Typographie et interface
- **Marcellus SC** (OFL) : noms de lieux, de boss, titres de bannière — capitales gravées, espacées.
- **Alegreya Sans** (OFL) : dialogues, invites, descriptions — humane et chaleureuse.
- Messages : cartouche sombre arrondi en bas d'écran ; invites : pastille parchemin avec la touche + verbe.
- Moins de texte : l'environnement raconte, le compagnon commente rarement (voir J8).

## 5. Reproductibilité
Après une régénération du niveau (`dress_jungle_2.py`), relancer `art_direction.py` puis `palette.py`.
`tune_pp.py` + `Saved/pp_params.json` servent aux itérations rapides ; les valeurs retenues sont recopiées dans `art_direction.py`.
