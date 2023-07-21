package main

import (
	"bufio"
	"encoding/csv"
	"encoding/json"
	"fmt"
	"log"
	"os"
)

type Logs2csv struct {
	LogTime           string `json:"log_time"`
	RemoteAddr        string `json:"remote_addr"`
	RemoteUser        string `json:"remote_user"`
	BodyBytesSent     string `json:"body_bytes_sent"`
	RequestTime       string `json:"request_time"`
	Status            string `json:"status"`
	Host              string `json:"host"`
	Request           string `json:"request"`
	RequestMethod     string `json:"request_method"`
	URI               string `json:"uri"`
	HTTPReferrer      string `json:"http_referrer"`
	HTTPXForwardedFor string `json:"http_x_forwarded_for"`
	HTTPUserAgent     string `json:"http_user_agent"`
}

const (
	bufferSize = 1024 * 1024 * 10 // 每次读取10MB大小的数据
)

func convertJSONToCSV(source, destination string) error {
	// 2. Read the JSON file into the struct array
	sourceFile, err := os.Open(source)
	if err != nil {
		return err
	}
	// remember to close the file at the end of the function
	defer sourceFile.Close()

	//大文件，需要分片处理，测试代码
	scanner := bufio.NewScanner(sourceFile)
	scanner.Buffer(make([]byte, bufferSize), bufferSize) // 设置扫描器缓冲区大小

	fmt.Println(scanner.Bytes())

	for scanner.Scan() {
		var data []interface{}
		err := json.Unmarshal(scanner.Bytes(), &data)
		if err != nil {
			fmt.Println("json解析错误", err)
			panic(err)
		}
		// 对读取到的数据进行处理
		fmt.Println(data)
	}
	//分片测试结束

	//小json 一次性读取处理方法
	var ranking []Logs2csv
	if err := json.NewDecoder(sourceFile).Decode(&ranking); err != nil {
		fmt.Println("错误：", err)
		return err
	}
	fmt.Println(ranking)

	// 3. Create a new file to store CSV data
	outputFile, err := os.Create(destination)
	if err != nil {
		return err
	}
	defer outputFile.Close()

	// 4. Write the header of the CSV file and the successive rows by iterating through the JSON struct array
	writer := csv.NewWriter(outputFile)
	defer writer.Flush()

	header := []string{"log_time", "remote_addr", "remote_user", "body_bytes_sent", "request_time", "status", "host", "request", "request_method", "uri", "http_referrer", "http_x_forwarded_for", "http_user_agent"}
	if err := writer.Write(header); err != nil {
		return err
	}

	for _, r := range ranking {
		var csvRow []string
		csvRow = append(csvRow, r.LogTime, r.RemoteAddr, r.RemoteUser, r.BodyBytesSent, r.RequestTime, r.Status, r.Host, r.Request, r.RequestMethod, r.URI, r.HTTPReferrer, r.HTTPXForwardedFor, r.HTTPUserAgent)
		if err := writer.Write(csvRow); err != nil {
			return err
		}
	}
	return nil
}

func main() {

	if err := convertJSONToCSV("log2.json", "log2.csv"); err != nil {
		log.Fatal(err)
	}

}
