using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Xml.Linq;

namespace Transitions
{
    public class XMLgenerator
    {
        
        public static string? InitialLandTypeId { get; set; }
        private static readonly List<string> AllowedAlphaTypes = new List<string> { "A_DR", "A_DL", "A_UU", "A_LL", "A_UR", "A_UL","B_UL", "B_UR", "B_DR", "B_DL", "B_UU", "B_LL" };



        public void GenerateXML(List<string> texture1FilePaths, List<string> texture2FilePaths, List<string> alphaImageFileNames, string outputPath, string nameTextureA, string brushIdA, string nameTextureB, string brushIdB)

        {
            XDocument xmlDocument = new XDocument(new XElement("transition"));

            string currentID = InitialLandTypeId; // Déclaré en dehors de l'appel à GenerateXML

            GenerateBrush(xmlDocument, nameTextureA, nameTextureB, ref currentID, texture1FilePaths, alphaImageFileNames.Where(x => x.Contains("A_")).ToList(), brushIdB, nameTextureB, brushIdA);
            GenerateBrush(xmlDocument, brushIdA, brushIdB, ref currentID, texture2FilePaths, alphaImageFileNames.Where(x => x.Contains("B_")).ToList(), nameTextureB, nameTextureA, nameTextureA);



            string xmlFilePath = Path.Combine(outputPath, "transition.xml");
            xmlDocument.Save(xmlFilePath);

            Console.WriteLine($"Le fichier XML a été généré avec succès : {xmlFilePath}");
        }

        private static void GenerateBrush(XDocument xmlDocument, string brushName, string brushId, ref string currentID, List<string> textureFilePaths, List<string> alphaImageFileNames, string oppositeBrushId, string nameTextureB, string nameTextureA)
        {
            if (xmlDocument.Root != null)
            {
                XElement brushElement = new XElement("Brush",
                    new XAttribute("Id", brushId),
                    new XAttribute("Name", brushName));

                foreach (string filePath in textureFilePaths)
                {
                    string textureName = Path.GetFileNameWithoutExtension(filePath);
                    string hexValue = ExtractHexNumber(textureName);

                    XElement landElement = new XElement("Land",
                        new XAttribute("ID", $"{hexValue}"));
                    brushElement.Add(landElement);
                }

                string nextID = IncrementHexID(currentID);
                bool isFirstTextureA = true; // Initialisez isFirstTextureA à true ou false en fonction de votre logique

                XElement edgeElement = new XElement("Edge");
                edgeElement.SetAttributeValue("To", oppositeBrushId);
                edgeElement.Add(new XComment($"{(isFirstTextureA ? nameTextureA : nameTextureB)}"));


            




                foreach (string alphaFileName in alphaImageFileNames)
                {
                    string alphaName = Path.GetFileNameWithoutExtension(alphaFileName);
                    string alphaType = ExtractAlphaType(alphaName);
                    string hexValue = ExtractHexNumber(alphaName);

                    // Supprimer les préfixes "A_" et "B_" de la valeur alphaType
                    if (alphaType.StartsWith("A_") || alphaType.StartsWith("B_"))
                    {
                        alphaType = alphaType.Substring(2); // Supprimer les deux premiers caractères (le préfixe)
                    }

                    // Créer l'élément <Land> en spécifiant l'attribut "Type" avec la valeur alphaType modifiée
                    XElement landElement = new XElement("Land",
                        new XAttribute("Type", alphaType),
                        new XAttribute("ID", $"0x{currentID}"));

                    edgeElement.Add(landElement);

                    currentID = IncrementHexID(currentID);
                }

                brushElement.Add(edgeElement);
                xmlDocument.Root.Add(brushElement);
            }
            else
            {
                Console.WriteLine("Attention : xmlDocument.Root est null. Vérifiez le chargement correct du document XML.");
            }

        }



        private static string ExtractHexNumber(string fileName)
        {
            return Regex.Match(fileName, @"\b0x[0-9a-fA-F]+\b").Value;
        }





        private static string ExtractAlphaType(string fileName)
        {
            string alphaType = fileName.Substring(0, 4);

            return AllowedAlphaTypes.Contains(alphaType) ? alphaType : "Unknown";

        }




        private static string IncrementHexID(string currentID)
        {
            // Convertir l'ID actuel en entier
            int intValue = Convert.ToInt32(currentID, 16);

            // Incrémenter l'entier de 1
            intValue++;

            // Convertir cet entier en représentation hexadécimale avec un format spécifique de 4 chiffres
            string nextHexID = intValue.ToString("X4");

            return nextHexID;
        }

    }
}


















