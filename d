set datafile separator ","
set title "Distribution des pourcentages"
set xlabel "Index"
set ylabel "Pourcentage (%)"
plot "donnees_ordinateur_1.csv" using 1:2 with linespoints title "Pourcentage"






set datafile separator ","

# Définir la précision pour les flottants
set format y "%.8f"
set format x "%.8f"

# Titre du graphique
set title "Analyse des Données"

# Étiquettes des axes
set xlabel "Index"
set ylabel "Pourcentage (%)"

# Lire les statistiques du fichier "donnees_ordinateur_1.csv"
stats "donnees_ordinateur_1.csv" using 2 name "stat_data"

# Afficher les statistiques
print "Max : ", stat_data_max
print "Min : ", stat_data_min
print "Moyenne générale : ", stat_data_mean
print "Moyenne entre Max et Min : ", (stat_data_max + stat_data_min) / 2
print "Médiane : ", stat_data_median
print "Aplatissement (Kurtosis) : ", stat_data_kurtosis
print "Asymétrie (Skewness) : ", stat_data_skewness
print "Somme des valeurs : ", stat_data_sum

# Tracer les données avec les statistiques
plot "donnees_ordinateur_1.csv" using 1:2 with linespoints title "Pourcentage"







set datafile separator ","

# Définir la précision pour les flottants
set format y "%.8f"
set format x "%.8f"

# Titre du graphique
set title "Analyse des Données"

# Étiquettes des axes
set xlabel "Index"
set ylabel "Pourcentage (%)"

# Lire les statistiques du fichier "donnees_ordinateur_1.csv"
stats "donnees_ordinateur_1.csv" using 2 name "stat_data"

# Afficher les statistiques dans la console
print "Max : ", stat_data_max
print "Min : ", stat_data_min
print "Moyenne générale : ", stat_data_mean
print "Moyenne entre Max et Min : ", (stat_data_max + stat_data_min) / 2
print "Médiane : ", stat_data_median
print "Aplatissement (Kurtosis) : ", stat_data_kurtosis
print "Asymétrie (Skewness) : ", stat_data_skewness
print "Somme des valeurs : ", stat_data_sum

# Afficher les informations sur le graphique
set label 1 at graph 0.5, 0.9 "Max: %.8f" offset 0,1 font ",12" textcolor rgb "blue" format stat_data_max
set label 2 at graph 0.5, 0.85 "Min: %.8f" offset 0,1 font ",12" textcolor rgb "blue" format stat_data_min
set label 3 at graph 0.5, 0.8 "Moyenne générale: %.8f" offset 0,1 font ",12" textcolor rgb "blue" format stat_data_mean
set label 4 at graph 0.5, 0.75 "Moyenne entre Max et Min: %.8f" offset 0,1 font ",12" textcolor rgb "blue" format (stat_data_max + stat_data_min) / 2
set label 5 at graph 0.5, 0.7 "Médiane: %.8f" offset 0,1 font ",12" textcolor rgb "blue" format stat_data_median
set label 6 at graph 0.5, 0.65 "Aplatissement (Kurtosis): %.8f" offset 0,1 font ",12" textcolor rgb "blue" format stat_data_kurtosis
set label 7 at graph 0.5, 0.6 "Asymétrie (Skewness): %.8f" offset 0,1 font ",12" textcolor rgb "blue" format stat_data_skewness
set label 8 at graph 0.5, 0.55 "Somme des valeurs: %.8f" offset 0,1 font ",12" textcolor rgb "blue" format stat_data_sum

# Tracer les données avec les statistiques
plot "donnees_ordinateur_1.csv" using 1:2 with linespoints title "Pourcentage"






set datafile separator ","

# Définir la précision pour les flottants
set format y "%.8f"
set format x "%.8f"

# Titre du graphique
set title "Analyse des Données"

# Étiquettes des axes
set xlabel "Index"
set ylabel "Pourcentage (%)"

# Lire les statistiques du fichier "donnees_ordinateur_1.csv"
stats "donnees_ordinateur_1.csv" using 2 name "stat_data"

# Afficher les statistiques dans la console
print "Max : ", stat_data_max
print "Min : ", stat_data_min
print "Moyenne générale : ", stat_data_mean
print "Moyenne entre Max et Min : ", (stat_data_max + stat_data_min) / 2

# Afficher les informations sur le graphique
set label 1 at graph 0.5, 0.9 "Max: %.8f" offset 0,1 font ",12" textcolor rgb "blue" format stat_data_max
set label 2 at graph 0.5, 0.85 "Min: %.8f" offset 0,1 font ",12" textcolor rgb "blue" format stat_data_min
set label 3 at graph 0.5, 0.8 "Moyenne générale: %.8f" offset 0,1 font ",12" textcolor rgb "blue" format stat_data_mean

# Tracer les données avec les statistiques
plot "donnees_ordinateur_1.csv" using 1:2 with linespoints title "Pourcentage"








set datafile separator ","

# Charger les statistiques
stats "donnees_ordinateur_1.csv" using 2 name "stat_data"

# Définir un titre et les labels des axes
set title "Distribution des Pourcentages"
set xlabel "Index"
set ylabel "Pourcentage (%)"

# Définir les labels des statistiques sur le graphique
set label 1 at graph 0.5, 0.9 sprintf("Max: %.8f", stat_data_max) offset 0,1 font ",12" textcolor rgb "blue"
set label 2 at graph 0.5, 0.85 sprintf("Min: %.8f", stat_data_min) offset 0,1 font ",12" textcolor rgb "red"
set label 3 at graph 0.5, 0.8 sprintf("Moyenne: %.8f", stat_data_mean) offset 0,1 font ",12" textcolor rgb "green"
set label 4 at graph 0.5, 0.75 sprintf("Mediane: %.8f", stat_data_median) offset 0,1 font ",12" textcolor rgb "purple"

# Tracer la courbe des données
plot "donnees_ordinateur_1.csv" using 1:2 with linespoints title "Pourcentage"






set datafile separator ","

# Charger les statistiques
stats "donnees_ordinateur_1.csv" using 2 name "stat_data"

# Définir un titre et les labels des axes
set title "Distribution des Pourcentages"
set xlabel "Index"
set ylabel "Pourcentage (%)"

# Définir les labels des statistiques sur le graphique
set label 3 at graph 0.5, 0.8 sprintf("MminMax: %.8f", (stat_data_max+stat_data_min)/2) offset 0,1 font ",8"

# Tracer la courbe des données
plot "donnees_ordinateur_1.csv" using 1:2 with linespoints title "Pourcentage"

